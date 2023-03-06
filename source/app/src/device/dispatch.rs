use std::sync::Arc;

use super::{adc::AdcHandle, dac::DacHandle, Error};
use crate::{
    channel::Channel,
    utils::{double_vec, AsyncCounter},
};
use async_std::task::{sleep, spawn};
use common::{
    config::{self, PointPortable, ADC_COUNT, DAC_COUNT},
    protocol::{self as proto, AppMsg, McuMsg, McuMsgRef},
};
use ferrite::atomic::AtomicVariable;
use flatty::{flat_vec, prelude::*};
use flatty_io::{AsyncReader as MsgReader, AsyncWriter as MsgWriter, ReadError};
use futures::{future::try_join_all, join};

pub struct Dispatcher<C: Channel> {
    reader: Reader<C>,
    writer: Writer<C>,
}

struct Writer<C: Channel> {
    channel: MsgWriter<AppMsg, C::Write>,
    dacs: [DacHandle; DAC_COUNT],
    dac_write_count: Arc<AsyncCounter>,
    dac_stream: double_vec::ReaderStream<i32>,
    dac_read_ready: Box<dyn FnMut()>,
}

struct Reader<C: Channel> {
    channel: MsgReader<McuMsg, C::Read>,
    adcs: [AdcHandle; ADC_COUNT],
    dac_write_count: Arc<AsyncCounter>,
}

impl<C: Channel> Dispatcher<C> {
    pub async fn new(
        channel: C,
        dacs: [DacHandle; DAC_COUNT],
        adcs: [AdcHandle; ADC_COUNT],
    ) -> Self {
        let (r, w) = channel.split();
        let reader = MsgReader::<McuMsg, _>::new(r, config::MAX_MCU_MSG_LEN);
        let writer = MsgWriter::<AppMsg, _>::new(w, config::MAX_APP_MSG_LEN);
        let dac_write_count = Arc::new(AsyncCounter::new(0));
        Self {
            reader: Reader {
                channel: reader,
                adcs,
            },
            writer: Writer {
                channel: writer,
                dacs,
            },
        }
    }
    pub async fn run(self) -> Result<(), Error> {
        try_join_all([spawn(self.reader.run()), spawn(self.writer.run())]).await;
        Ok(())
    }
}

impl<C: Channel> Reader<C> {
    async fn run(self) -> Result<(), Error> {
        let mut channel = self.channel;
        let mut adcs = self.adcs;
        loop {
            let msg = match channel.read_message().await {
                Err(ReadError::Eof) => return Err(Error::Disconnected),
                other => other.unwrap(),
            };
            match msg.as_ref() {
                McuMsgRef::DinUpdate { value: _ } => (),
                McuMsgRef::DacRequest { count } => self.dacs[0].request(count.to_native() as usize),
                McuMsgRef::AdcData { points } => {
                    for (index, adc) in adcs.iter_mut().enumerate() {
                        adc.push(points.iter().map(|a| a[index].to_native())).await;
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

impl<C: Channel> Writer<C> {
    async fn run(mut self) -> Result<(), Error> {
        {
            self.channel
                .new_message()
                .emplace(proto::AppMsgInitConnect)
                .unwrap()
                .write()
                .await
                .unwrap();
        }
        loop {
            sleep(config::KEEP_ALIVE_PERIOD).await;
            self.channel
                .new_message()
                .emplace(proto::AppMsgInitKeepAlive)
                .unwrap()
                .write()
                .await
                .unwrap();
        }
    }
}

struct PointSender<C: Channel> {
    channel: MsgWriter<AppMsg, C::Write>,
    request: Arc<AtomicVariable<u16>>,
}

impl<C: Channel> PointSender<C> {
    async fn run(mut self) {
        self.stream.on_swap = Box::new({
            let req = self.request.clone();
            move || req.store(1)
        });

        self.request.store(1);
        loop {
            join!(self.points_to_send.wait(1), async {
                if self.stream.is_empty() {
                    self.stream.wait_ready().await;
                }
            });

            let mut msg = self
                .channel
                .new_message()
                .emplace(proto::AppMsgInitDacData {
                    points: flat_vec![],
                })
                .unwrap();
            let will_send = if let proto::AppMsgMut::DacData { points } = msg.as_mut() {
                let mut count = self.points_to_send.sub(None);
                //log::debug!("points_to_send: {}", count);
                while count > 0 && !points.is_full() {
                    match self.stream.next().await {
                        Some(value) => {
                            points.push([PointPortable::from_native(value)]).unwrap();
                            count -= 1;
                        }
                        None => break,
                    }
                }
                //log::debug!("points_send: {}", msg.points.len());
                self.points_to_send.add(count);
                !points.is_empty()
            } else {
                unreachable!();
            };
            if will_send {
                msg.write().await.unwrap();
            }
        }
    }
}
