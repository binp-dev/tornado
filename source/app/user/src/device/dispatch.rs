use super::{
    ai::AiHandle,
    ao::AoHandle,
    debug::DebugHandle,
    dio::{DiHandle, DoHandle},
    Error,
};
use crate::{channel::Channel, wf::WfData};
use async_atomic::{Atomic as AsyncAtomic, Subscriber};
use async_compat::Compat;
use common::{
    config::{self, AI_COUNT},
    protocol::{self as proto, AppMsg, McuMsg, McuMsgRef},
    values::Point,
};
use flatty::{flat_vec, prelude::*, vec::FromIterator, Emplacer};
use flatty_io::{AsyncReader as MsgReader, AsyncWriter as MsgWriter, ReadError};
use futures::{future::try_join_all, join, AsyncWrite, FutureExt, SinkExt, StreamExt};
use std::{io, path::Path, sync::Arc};
use tokio::{
    spawn,
    sync::{Mutex, Notify},
    time::sleep,
};

pub struct Dispatcher<C: Channel> {
    writer: Writer<C>,
    reader: Reader<C>,
}

struct Writer<C: Channel> {
    channel: Mutex<MsgWriter<AppMsg, Compat<C::Write>>>,
    ao: AoHandle,
    ao_write_count: Subscriber<usize>,
    do_: DoHandle,
    debug: DebugHandle,
    wf_data: Arc<WfData>,
    ping_pong: Arc<Notify>,
}

struct Reader<C: Channel> {
    channel: MsgReader<McuMsg, Compat<C::Read>>,
    ais: [AiHandle; AI_COUNT],
    ao_write_count: Arc<AsyncAtomic<usize>>,
    di: DiHandle,
    wf_data: Arc<WfData>,
    ping_pong: Arc<Notify>,
}

impl<C: Channel> Dispatcher<C> {
    pub async fn new(
        channel: C,
        ao: AoHandle,
        ais: [AiHandle; AI_COUNT],
        di: DiHandle,
        do_: DoHandle,
        debug: DebugHandle,
    ) -> Self {
        let (r, w) = channel.split();
        let (r, w) = (Compat::new(r), Compat::new(w));
        let reader = MsgReader::<McuMsg, _>::new(r, config::MAX_MCU_MSG_LEN);
        let writer = Mutex::new(MsgWriter::<AppMsg, _>::new(w, config::MAX_APP_MSG_LEN));
        let ao_write_count = AsyncAtomic::new(0).subscribe();
        let wf_data = Arc::new(WfData::new(Path::new("/dev/uio0")).unwrap());
        let ping_pong = Arc::new(Notify::new());
        Self {
            reader: Reader {
                channel: reader,
                ais,
                ao_write_count: ao_write_count.clone(),
                di,
                wf_data: wf_data.clone(),
                ping_pong: ping_pong.clone(),
            },
            writer: Writer {
                channel: writer,
                ao,
                ao_write_count,
                do_,
                debug,
                wf_data,
                ping_pong,
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
        let mut ais = self.ais;
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
                McuMsgRef::DiUpdate { value } => self.di.send(*value).await.unwrap(),
                McuMsgRef::AoRequest { count } => {
                    self.ao_write_count.fetch_add(*count as usize);
                }
                McuMsgRef::AiData { points } => {
                    for (index, ai) in ais.iter_mut().enumerate() {
                        ai.push_iter(points.iter().map(|a| a[index])).await;
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
                McuMsgRef::WfBufTest { offset, value } => {
                    println!("Read Wf buffer at {}", offset);
                    let data = unsafe { self.wf_data.read(*offset as usize, value.len()) };
                    assert_eq!(*data, value.as_slice());
                    self.ping_pong.notify_one();
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
                    for j in 0.. {
                        let offset = 0;
                        let len = 128;
                        let mut data = unsafe { self.wf_data.write(offset, len) };
                        for (i, b) in data.iter_mut().enumerate() {
                            *b = ((len - i + j) % 256) as u8;
                        }
                        println!("Write wf buffer at {}", offset);

                        let mut guard = channel.lock().await;
                        let umsg = guard.alloc_message();
                        let msg = umsg
                            .new_in_place(proto::AppMsgInitWfBufTest {
                                offset: offset as u32,
                                value: FromIterator(data.iter().copied()),
                            })
                            .unwrap();

                        drop(data);
                        msg.write().await?;

                        self.ping_pong.notified().await;
                    }
                    Ok(())
                }
            })
            .map(Result::unwrap),
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
                        let value = self.do_.next().await.unwrap();
                        send_message(&channel, proto::AppMsgInitDoUpdate { value }).await?;
                    }
                }
            })
            .map(Result::unwrap),
            spawn({
                let channel = channel.clone();
                async move {
                    loop {
                        let value = self.ao.add.next().await.unwrap();
                        send_message(&channel, proto::AppMsgInitAoAdd { value }).await?;
                    }
                }
            })
            .map(Result::unwrap),
            spawn(async move {
                let mut iter = self.ao.buffer;
                loop {
                    join!(self.ao_write_count.wait(|x| x >= 1), iter.wait_ready());
                    let mut guard = channel.lock().await;
                    let mut msg = guard
                        .alloc_message()
                        .new_in_place(proto::AppMsgInitAoData {
                            points: flat_vec![],
                        })
                        .unwrap();
                    let will_send = if let proto::AppMsgMut::AoData { points } = msg.as_mut() {
                        let mut count = self.ao_write_count.swap(0);
                        while count > 0 && !points.is_full() {
                            match iter.next() {
                                Some(value) => {
                                    points.push(Point::from_uv(value)).unwrap();
                                    count -= 1;
                                }
                                None => break,
                            }
                        }
                        self.ao_write_count.fetch_add(count);
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
