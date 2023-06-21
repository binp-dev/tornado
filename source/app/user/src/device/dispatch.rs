use super::{
    adc::AdcHandle,
    dac::DacHandle,
    debug::DebugHandle,
    dio::{DinHandle, DoutHandle},
    Error,
};
use crate::channel::Channel;
use async_atomic::{Atomic as AsyncAtomic, Subscriber};
use async_compat::Compat;
use common::{
    config::{self, ADC_COUNT},
    protocol::{self as proto, AppMsg, McuMsg, McuMsgRef},
    values::Point,
};
use flatty::{flat_vec, prelude::*, Emplacer};
use flatty_io::{AsyncReader as MsgReader, AsyncWriter as MsgWriter, ReadError};
use futures::{future::try_join_all, join, AsyncWrite, FutureExt, SinkExt, StreamExt};
use std::{io, sync::Arc};
use tokio::{spawn, sync::Mutex, time::sleep};

pub struct Dispatcher<C: Channel> {
    writer: Writer<C>,
    reader: Reader<C>,
}

struct Writer<C: Channel> {
    channel: Mutex<MsgWriter<AppMsg, Compat<C::Write>>>,
    dac: DacHandle,
    dac_write_count: Subscriber<usize>,
    dout: DoutHandle,
    debug: DebugHandle,
}

struct Reader<C: Channel> {
    channel: MsgReader<McuMsg, Compat<C::Read>>,
    adcs: [AdcHandle; ADC_COUNT],
    dac_write_count: Arc<AsyncAtomic<usize>>,
    din: DinHandle,
}

impl<C: Channel> Dispatcher<C> {
    pub async fn new(
        channel: C,
        dac: DacHandle,
        adcs: [AdcHandle; ADC_COUNT],
        din: DinHandle,
        dout: DoutHandle,
        debug: DebugHandle,
    ) -> Self {
        let (r, w) = channel.split();
        let (r, w) = (Compat::new(r), Compat::new(w));
        let reader = MsgReader::<McuMsg, _>::new(r, config::MAX_MCU_MSG_LEN);
        let writer = Mutex::new(MsgWriter::<AppMsg, _>::new(w, config::MAX_APP_MSG_LEN));
        let dac_write_count = AsyncAtomic::new(0).subscribe();
        Self {
            reader: Reader {
                channel: reader,
                adcs,
                dac_write_count: dac_write_count.clone(),
                din,
            },
            writer: Writer {
                channel: writer,
                dac,
                dac_write_count,
                dout,
                debug,
            },
        }
    }
    pub async fn run(self) -> Result<(), Error> {
        try_join_all([
            spawn(self.reader.run()).map(Result::unwrap),
            spawn(self.writer.run()).map(Result::unwrap),
        ])
        .await
        .map(|_| ())
    }
}

impl<C: Channel> Reader<C> {
    async fn run(mut self) -> Result<(), Error> {
        let mut channel = self.channel;
        let mut adcs = self.adcs;
        loop {
            let msg = match channel.read_message().await {
                Err(ReadError::Eof) => break Err(Error::Disconnected),
                Err(ReadError::Io(err)) => {
                    if err.kind() == io::ErrorKind::ConnectionReset {
                        break Err(Error::Disconnected);
                    } else {
                        panic!("I/O error: {}", err);
                    }
                }
                other => other.unwrap(),
            };
            match msg.as_ref() {
                McuMsgRef::DinUpdate { value } => self.din.send(*value).await.unwrap(),
                McuMsgRef::DacRequest { count } => {
                    self.dac_write_count.fetch_add(*count as usize);
                }
                McuMsgRef::AdcData { points } => {
                    for (index, adc) in adcs.iter_mut().enumerate() {
                        adc.push_iter(points.iter().map(|a| a[index])).await;
                    }
                }
                McuMsgRef::Error { code, message } => {
                    panic!(
                        "Error {}: {}",
                        code,
                        String::from_utf8_lossy(message.as_slice())
                    )
                }
                McuMsgRef::Debug { message } => {
                    println!("Debug: {}", String::from_utf8_lossy(message.as_slice()))
                }
            }
        }
    }
}

async fn send_message<M: Flat + ?Sized, W: AsyncWrite + Unpin, E: Emplacer<M>>(
    channel: &Mutex<MsgWriter<M, W>>,
    emplacer: E,
) -> Result<(), io::Error> {
    channel
        .lock()
        .await
        .alloc_message()
        .new_in_place(emplacer)
        .unwrap()
        .write()
        .await
}

impl<C: Channel> Writer<C> {
    async fn run(mut self) -> Result<(), Error> {
        let channel = Arc::new(self.channel);
        let res: Result<Vec<()>, io::Error> = try_join_all([
            spawn({
                let channel = channel.clone();
                async move {
                    loop {
                        send_message(&channel, proto::AppMsgInitKeepAlive).await?;
                        sleep(config::KEEP_ALIVE_PERIOD).await;
                    }
                }
            })
            .map(Result::unwrap),
            spawn({
                let channel = channel.clone();
                async move {
                    loop {
                        send_message(&channel, proto::AppMsgInitStatsReset).await?;
                        self.debug.stats_reset.next().await;
                    }
                }
            })
            .map(Result::unwrap),
            spawn({
                let channel = channel.clone();
                async move {
                    loop {
                        let value = self.dout.next().await.unwrap();
                        send_message(&channel, proto::AppMsgInitDoutUpdate { value }).await?;
                    }
                }
            })
            .map(Result::unwrap),
            spawn({
                let channel = channel.clone();
                async move {
                    loop {
                        let value = self.dac.addition.next().await.unwrap();
                        send_message(&channel, proto::AppMsgInitDacAdd { value }).await?;
                    }
                }
            })
            .map(Result::unwrap),
            spawn(async move {
                let mut iter = self.dac.buffer;
                loop {
                    join!(self.dac_write_count.wait(|x| x >= 1), iter.wait_ready());
                    let mut guard = channel.lock().await;
                    let mut msg = guard
                        .alloc_message()
                        .new_in_place(proto::AppMsgInitDacData {
                            points: flat_vec![],
                        })
                        .unwrap();
                    let will_send = if let proto::AppMsgMut::DacData { points } = msg.as_mut() {
                        let mut count = self.dac_write_count.swap(0);
                        while count > 0 && !points.is_full() {
                            match iter.next() {
                                Some(value) => {
                                    points.push(Point::from_uv(value)).unwrap();
                                    count -= 1;
                                }
                                None => break,
                            }
                        }
                        self.dac_write_count.fetch_add(count);
                        !points.is_empty()
                    } else {
                        unreachable!();
                    };
                    if will_send {
                        msg.write().await?;
                    }
                }
            })
            .map(Result::unwrap),
        ])
        .await;
        match res {
            Ok(_) => Ok(()),
            Err(err) => match err.kind() {
                io::ErrorKind::BrokenPipe => Err(Error::Disconnected),
                other => panic!("I/O Error: {}", other),
            },
        }
    }
}
