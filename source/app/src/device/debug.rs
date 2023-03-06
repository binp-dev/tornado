use crate::{channel::Channel, epics};
use common::protocol::{self as proto, AppMsg};
use flatty_io::AsyncWriter as MsgWriter;

pub struct Debug<C: Channel> {
    pub epics: epics::Debug,
    pub channel: MsgWriter<AppMsg, C::Write>,
}

impl<C: Channel> Debug<C> {
    async fn send(&mut self) {
        self.channel
            .new_message()
            .emplace(proto::AppMsgInitStatsReset)
            .unwrap()
            .write()
            .await
            .unwrap();
    }

    pub async fn run(mut self) {
        self.send().await;
        loop {
            if self.epics.stats_reset.wait().await.read().await != 0 {
                self.send().await;
            }
        }
    }
}
