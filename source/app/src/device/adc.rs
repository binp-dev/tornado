use crate::{config::Point, epics};
use async_ringbuf::{AsyncHeapRb, AsyncProducer};
use ferrite::VarSync;
use std::{future::Future, iter::ExactSizeIterator, sync::Arc};

pub struct Adc {
    pub epics: epics::Adc,
}

pub struct AdcHandle {
    buffer: AsyncProducer<Point, Arc<AsyncHeapRb<Point>>>,
}

impl Adc {
    pub fn run(self) -> (impl Future<Output = ()>, AdcHandle) {
        let mut epics = self.epics;
        let max_len = epics.array.max_len();
        let buffer = AsyncHeapRb::<Point>::new(2 * max_len);
        let (producer, mut consumer) = buffer.split();
        (
            async move {
                loop {
                    consumer.wait(max_len).await;
                    assert!(consumer.len() >= max_len);
                    let buffer = consumer.as_mut_base();
                    epics
                        .array
                        .request()
                        .await
                        .write_from(buffer.pop_iter().map(|x| x as f64))
                        .await;
                }
            },
            AdcHandle { buffer: producer },
        )
    }
}

impl AdcHandle {
    pub async fn push<I: ExactSizeIterator<Item = Point>>(&mut self, points: I) {
        self.buffer.push_iter(points).await.ok().unwrap()
    }
}
