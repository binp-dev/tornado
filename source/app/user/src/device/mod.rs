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
use dio::{Di, Do};
use dispatch::Dispatcher;
use tokio::spawn;

#[derive(Clone, Debug)]
pub enum Error {
    Disconnected,
}

pub struct Device<C: Channel> {
    ao: Ao,
    ais: [Ai; config::AI_COUNT],
    di: Di,
    do_: Do,
    dispatcher: Dispatcher<C>,
}

impl<C: Channel> Device<C> {
    pub async fn new(channel: C, epics: Epics) -> Self {
        let (ao, ao_handle) = Ao::new(epics.ao);
        let (ais, ai_handles) = unzip_array(epics.ais.map(Ai::new));
        let (di, di_handle) = Di::new(epics.di);
        let (do_, do_handle) = Do::new(epics.do_);
        let debug_handle = Debug::new(epics.debug);
        let dispatcher = Dispatcher::new(
            channel,
            ao_handle,
            ai_handles,
            di_handle,
            do_handle,
            debug_handle,
        )
        .await;
        Self {
            ao,
            ais,
            di,
            do_,
            dispatcher,
        }
    }

    pub async fn run(self) {
        let res = try_join_all([
            spawn(self.ao.run()).map(Result::unwrap),
            spawn(try_join_all(self.ais.map(|adc| adc.run())).map(|r| r.map(|_| ())))
                .map(Result::unwrap),
            spawn(self.di.run()).map(Result::unwrap),
            spawn(self.do_.run()).map(Result::unwrap),
            spawn(self.dispatcher.run()).map(Result::unwrap),
        ])
        .await;
        log::warn!("Stopping device: {:?}", res);
    }
}
