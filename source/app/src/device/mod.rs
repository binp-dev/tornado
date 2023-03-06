mod adc;
mod dac;
mod debug;
mod dispatch;

use crate::{channel::Channel, epics::Epics};
use async_std::task::{sleep, spawn};
use common::{
    config,
    protocol::{self as proto, AppMsg, McuMsg, McuMsgRef},
};
use flatty::prelude::*;
use flatty_io::{AsyncReader as MsgReader, AsyncWriter as MsgWriter};
use futures::future::{try_join_all, FutureExt};

use adc::{Adc, AdcHandle};
use dac::{Dac, DacHandle};
use debug::Debug;
use dispatch::Dispatcher;

enum Error {
    Disconnected,
}

pub struct Device<C: Channel> {
    reader: MsgReader<McuMsg, C::Read>,
    writer: MsgWriter<AppMsg, C::Write>,
    dacs: [Dac<C>; config::DAC_COUNT],
    adcs: [Adc; config::ADC_COUNT],
    debug: Debug<C>,
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

    pub async fn run(self) {
        let (dac_loops, dac_handles): (Vec<_>, Vec<_>) =
            self.dacs.into_iter().map(|dac| dac.run()).unzip();
        let (adc_loops, adc_handles): (Vec<_>, Vec<_>) =
            self.adcs.into_iter().map(|adc| adc.run()).unzip();

        let dispatcher = Dispatcher::<C> {
            reader: dispatch::Reader {
                channel: self.reader,
                dacs: dac_handles.try_into().ok().unwrap(),
                adcs: adc_handles.try_into().ok().unwrap(),
            },
            writer: dispatch::Writer {
                channel: self.writer,
            },
        };

        let res = try_join_all([
            spawn(try_join_all(dac_loops).map(|r| r.map(|_| ()))),
            spawn(try_join_all(adc_loops).map(|r| r.map(|_| ()))),
            spawn(dispatcher.run()),
            spawn(keepalive.run()),
            spawn(self.debug.run()),
        ])
        .await;
    }
}
