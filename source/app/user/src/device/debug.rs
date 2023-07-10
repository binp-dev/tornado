use std::pin::Pin;

use crate::epics;
use futures::{Stream, StreamExt};

pub enum Debug {}

pub struct DebugHandle {
    pub stats_reset: Pin<Box<dyn Stream<Item = ()> + Send>>,
}

impl Debug {
    #[allow(clippy::new_ret_no_self)]
    pub fn new(epics: epics::Debug) -> DebugHandle {
        DebugHandle {
            stats_reset: Box::pin(epics.reset_stats.into_stream().filter_map(|x| async move {
                if x != 0 {
                    Some(())
                } else {
                    None
                }
            })),
        }
    }
}
