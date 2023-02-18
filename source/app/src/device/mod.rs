mod adc;
mod dac;
mod debug;

use crate::{channel::Channel, epics::Epics};
use async_std::{sync::Arc, task::sleep};
use common::{
    config,
    protocol::{self as proto, AppMsg, McuMsg, McuMsgRef},
};
use flatty::prelude::*;
use flatty_io::{AsyncReader as MsgReader, AsyncWriter as MsgWriter};
use futures::{
    executor::ThreadPool,
    future::{join_all, FutureExt},
};

use adc::{Adc, AdcHandle};
use dac::{Dac, DacHandle};
use debug::Debug;

pub struct Device<C: Channel> {
    reader: MsgReader<McuMsg, C::Read>,
    writer: MsgWriter<AppMsg, C::Write>,
    dacs: [Dac<C>; config::DAC_COUNT],
    adcs: [Adc; config::ADC_COUNT],
    debug: Debug<C>,
}

struct MsgDispatcher<C: Channel> {
    channel: MsgReader<McuMsg, C::Read>,
    dacs: [DacHandle; config::DAC_COUNT],
    adcs: [AdcHandle; config::ADC_COUNT],
}

struct Keepalive<C: Channel> {
    channel: MsgWriter<AppMsg, C::Write>,
}

impl<C: Channel> Device<C> {
    pub fn new(channel: C, epics: Epics) -> Self {
        let (br, bw) = channel.split();
        let reader = MsgReader::<McuMsg, _>::new(br, config::MAX_MCU_MSG_LEN);
        let writer = MsgWriter::<AppMsg, _>::new(bw, config::MAX_APP_MSG_LEN);
        Self {
            dacs: epics.dac.map(|epics| Dac {
                channel: writer.clone(),
                epics,
            }),
            adcs: epics.adc.map(|epics| Adc { epics }),
            debug: Debug {
                epics: epics.debug,
                channel: writer.clone(),
            },
            reader,
            writer,
        }
    }

    pub async fn run(self, exec: Arc<ThreadPool>) {
        let (dac_loops, dac_handles): (Vec<_>, Vec<_>) = self
            .dacs
            .into_iter()
            .map(|dac| dac.run(exec.clone()))
            .unzip();
        let (adc_loops, adc_handles): (Vec<_>, Vec<_>) =
            self.adcs.into_iter().map(|adc| adc.run()).unzip();

        let dispatcher = MsgDispatcher::<C> {
            channel: self.reader,
            dacs: dac_handles.try_into().ok().unwrap(),
            adcs: adc_handles.try_into().ok().unwrap(),
        };
        let keepalive = Keepalive::<C> {
            channel: self.writer.clone(),
        };

        exec.spawn_ok(join_all(dac_loops).map(|_| ()));
        exec.spawn_ok(join_all(adc_loops).map(|_| ()));
        exec.spawn_ok(dispatcher.run());
        exec.spawn_ok(keepalive.run());
        exec.spawn_ok(self.debug.run());
    }
}

impl<C: Channel> MsgDispatcher<C> {
    async fn run(self) {
        let mut channel = self.channel;
        let mut adcs = self.adcs;
        loop {
            let msg = channel.read_message().await.unwrap();
            match msg.as_ref() {
                McuMsgRef::DinUpdate { value: _ } => (),
                McuMsgRef::DacRequest { count } => self.dacs[0].request(count.to_native() as usize),
                McuMsgRef::AdcData { points } => {
                    for (index, adc) in adcs.iter_mut().enumerate() {
                        adc.push(points.iter().map(|a| a[index].to_native())).await
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

impl<C: Channel> Keepalive<C> {
    async fn run(mut self) {
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
