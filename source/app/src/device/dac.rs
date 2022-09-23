use crate::{channel::Channel, epics, proto::AppMsg};
use async_std::sync::{Arc, Mutex};
use ferrite::channel::MsgWriter;
use std::{
    future::Future,
    rc::Rc,
    sync::atomic::{self, AtomicUsize},
};

pub struct Dac {
    pub channel: Arc<Mutex<MsgWriter<AppMsg, Channel>>>,
    pub epics: epics::Dac,
}

pub struct DacHandle {
    request_counter: Rc<AtomicUsize>,
}

impl Dac {
    pub fn run(self) -> (impl Future<Output = ()>, DacHandle) {
        let counter = Rc::new(AtomicUsize::new(0));
        (
            async move {},
            DacHandle {
                request_counter: counter,
            },
        )
    }
}

impl DacHandle {
    pub fn request(&self, count: usize) {
        self.request_counter.fetch_add(count, atomic::Ordering::SeqCst);
    }
}
