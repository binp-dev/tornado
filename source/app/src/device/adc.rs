use super::Error;
use crate::epics;
use async_ringbuf::{AsyncConsumer, AsyncHeapRb, AsyncProducer};
use common::config::{adc_to_volt, Point};
use ferrite::TypedVariable as Variable;
use std::{iter::ExactSizeIterator, sync::Arc};

pub struct Adc {
    input: AsyncConsumer<Point, Arc<AsyncHeapRb<Point>>>,
    output_array: Variable<[f64]>,
}

pub struct AdcHandle {
    pub buffer: AsyncProducer<Point, Arc<AsyncHeapRb<Point>>>,
}

impl Adc {
    pub fn new(epics: epics::Adc) -> (Self, AdcHandle) {
        let buffer = AsyncHeapRb::<Point>::new(2 * epics.array.max_len());
        let (producer, consumer) = buffer.split();
        (
            Self {
                input: consumer,
                output_array: epics.array,
            },
            AdcHandle { buffer: producer },
        )
    }
    pub async fn run(mut self) -> Result<(), Error> {
        let max_len = self.output_array.max_len();
        loop {
            self.input.wait(max_len).await;
            if self.input.is_closed() {
                break Err(Error::Disconnected);
            }
            assert!(self.input.len() >= max_len);
            let input = self.input.as_mut_base();
            self.output_array
                .request()
                .await
                .write_from(input.pop_iter().map(adc_to_volt))
                .await;
        }
    }
}

impl AdcHandle {
    pub async fn push_iter<I: ExactSizeIterator<Item = Point>>(&mut self, points: I) {
        self.buffer.push_iter(points).await.ok().unwrap()
    }
}
