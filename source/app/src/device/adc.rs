use crate::{config::Point, epics};
use async_ringbuf::{AsyncHeapRb, AsyncProducer};
use std::{future::Future, iter::ExactSizeIterator, sync::Arc};

pub struct Adc {
    pub epics: epics::Adc,
}

pub struct AdcHandle {
    buffer: AsyncProducer<Point, Arc<AsyncHeapRb<Point>>>,
}

impl Adc {
    pub fn run(self) -> (impl Future<Output = ()>, AdcHandle) {
        let buffer = AsyncHeapRb::<Point>::new(self.epics.array.max_len());
        let (producer, consumer) = buffer.split();
        (async move {}, AdcHandle { buffer: producer })
    }
}

impl AdcHandle {
    pub async fn push<I: ExactSizeIterator<Item = Point>>(&mut self, points: I) {
        self.buffer.push_iter(points).await.ok().unwrap()
    }
}
