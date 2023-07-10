mod ai;
mod ao;
mod debug;
mod dio;
mod dispatch;

use crate::{channel::Channel, epics::Epics, utils::misc::unzip_array};
use common::config;
use futures::future::{try_join_all, FutureExt};

use ai::Ai;
use ao::Ao;
use debug::Debug;
use dio::{Din, Dout};
use dispatch::Dispatcher;
use tokio::spawn;

#[derive(Clone, Debug)]
pub enum Error {
    Disconnected,
}

pub struct Device<C: Channel> {
    dac: Ao,
    adcs: [Ai; config::AI_COUNT],
    din: Din,
    dout: Dout,
    dispatcher: Dispatcher<C>,
}

impl<C: Channel> Device<C> {
    pub async fn new(channel: C, epics: Epics) -> Self {
        let (dac, dac_handle) = Ao::new(epics.ao);
        let (adcs, adc_handles) = unzip_array(epics.ai.map(Ai::new));
        let (din, din_handle) = Din::new(epics.di);
        let (dout, dout_handle) = Dout::new(epics.do_);
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
            spawn(self.dac.run()).map(Result::unwrap),
            spawn(try_join_all(self.adcs.map(|adc| adc.run())).map(|r| r.map(|_| ())))
                .map(Result::unwrap),
            spawn(self.din.run()).map(Result::unwrap),
            spawn(self.dout.run()).map(Result::unwrap),
            spawn(self.dispatcher.run()).map(Result::unwrap),
        ])
        .await;
        log::warn!("Stopping device: {:?}", res);
    }
}
