mod adc;
mod dac;
mod debug;
mod dio;
mod dispatch;

use crate::{channel::Channel, epics::Epics, utils::misc::unzip_array};
use async_std::task::spawn;
use common::config;
use futures::future::{try_join_all, FutureExt};

use adc::Adc;
use dac::Dac;
use debug::Debug;
use dio::{Din, Dout};
use dispatch::Dispatcher;

#[derive(Clone, Debug)]
pub enum Error {
    Disconnected,
}

pub struct Device<C: Channel> {
    dac: Dac,
    adcs: [Adc; config::ADC_COUNT],
    din: Din,
    dout: Dout,
    dispatcher: Dispatcher<C>,
}

impl<C: Channel> Device<C> {
    pub async fn new(channel: C, epics: Epics) -> Self {
        let (dac, dac_handle) = Dac::new(epics.dac);
        let (adcs, adc_handles) = unzip_array(epics.adc.map(Adc::new));
        let (din, din_handle) = Din::new(epics.din);
        let (dout, dout_handle) = Dout::new(epics.dout);
        let debug_handle = Debug::new(epics.debug);
        let dispatcher = Dispatcher::new(
            channel,
            dac_handle,
            adc_handles,
            din_handle,
            dout_handle,
            debug_handle,
        )
        .await;
        Self {
            dac,
            adcs,
            din,
            dout,
            dispatcher,
        }
    }

    pub async fn run(self) {
        let res = try_join_all([
            spawn(self.dac.run()),
            spawn(try_join_all(self.adcs.map(|adc| adc.run())).map(|r| r.map(|_| ()))),
            spawn(self.din.run()),
            spawn(self.dout.run()),
            spawn(self.dispatcher.run()),
        ])
        .await;
        log::warn!("Stopping device: {:?}", res);
    }
}
