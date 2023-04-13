use super::Error;
use crate::epics;
use async_ringbuf::{AsyncHeapConsumer, AsyncHeapProducer, AsyncHeapRb};
use common::values::{Point, Value};
use ferrite::TypedVariable as Variable;
use futures::{future::try_join_all, FutureExt};
use std::{
    iter::ExactSizeIterator,
    sync::{atomic::Ordering, Arc},
};
use tokio::spawn;

pub struct Adc {
    array: AdcArray,
    scalar: AdcScalar,
}

struct AdcArray {
    input: AsyncHeapConsumer<Point>,
    output: Variable<[f64]>,
}

struct AdcScalar {
    input: Arc<<Point as Value>::Atomic>,
    output: Variable<f64>,
}

pub struct AdcHandle {
    buffer: AsyncHeapProducer<Point>,
    last_point: Arc<<Point as Value>::Atomic>,
}

impl Adc {
    pub fn new(epics: epics::Adc) -> (Self, AdcHandle) {
        let buffer = AsyncHeapRb::<Point>::new(2 * epics.array.max_len());
        let (producer, consumer) = buffer.split();
        let last = Arc::new(<Point as Value>::Atomic::default());
        (
            Self {
                array: AdcArray {
                    input: consumer,
                    output: epics.array,
                },
                scalar: AdcScalar {
                    input: last.clone(),
                    output: epics.scalar,
                },
            },
            AdcHandle {
                buffer: producer,
                last_point: last,
            },
        )
    }
    pub async fn run(self) -> Result<(), Error> {
        try_join_all([
            spawn(self.array.run()).map(Result::unwrap),
            spawn(self.scalar.run()).map(Result::unwrap),
        ])
        .await
        .map(|_| ())
    }
}

impl AdcArray {
    async fn run(mut self) -> Result<(), Error> {
        let max_len = self.output.max_len();
        loop {
            self.input.wait(max_len).await;
            if self.input.is_closed() {
                break Err(Error::Disconnected);
            }
            assert!(self.input.len() >= max_len);
            let input = self.input.as_mut_base();
            self.output
                .request()
                .await
                .write_from(input.pop_iter().map(Point::into_analog))
                .await;
        }
    }
}

impl AdcScalar {
    async fn run(mut self) -> Result<(), Error> {
        loop {
            self.output
                .wait()
                .await
                .write(
                    Point::try_from_base(self.input.load(Ordering::Acquire))
                        .unwrap()
                        .into_analog(),
                )
                .await;
        }
    }
}

impl AdcHandle {
    pub async fn push_iter<I: ExactSizeIterator<Item = Point>>(&mut self, points: I) {
        let mut last = Point::default();
        let len = points.len();
        self.buffer
            .push_iter(points.map(|x| {
                last = x;
                x
            }))
            .await
            .ok()
            .unwrap();
        if len > 0 {
            self.last_point.store(last.into_base(), Ordering::Release);
        }
    }
}
