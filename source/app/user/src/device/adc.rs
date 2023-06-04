use super::Error;
use crate::epics;
use async_ringbuf::{traits::*, AsyncHeapRb};
use common::values::{uv_to_volt, AtomicUv, Point, PointOpt, Uv};
use ferrite::TypedVariable as Variable;
use futures::{future::try_join_all, FutureExt};
use ringbuf::traits::*;
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
    input: <AsyncHeapRb<Point> as Split>::Cons,
    output: Variable<[f64]>,
}

struct AdcScalar {
    input: Arc<AtomicUv>,
    output: Variable<f64>,
}

pub struct AdcHandle {
    buffer: <AsyncHeapRb<Point> as Split>::Prod,
    last_point: Arc<AtomicUv>,
}

impl Adc {
    pub fn new(epics: epics::Adc) -> (Self, AdcHandle) {
        let buffer = AsyncHeapRb::<Point>::new(2 * epics.array.max_len());
        let (producer, consumer) = buffer.split();
        let last = Arc::new(AtomicUv::default());
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
            self.input.wait_occupied(max_len).await;
            if self.input.is_closed() {
                break Err(Error::Disconnected);
            }
            assert!(self.input.occupied_len() >= max_len);
            self.output
                .request()
                .await
                .write_from(self.input.pop_iter().filter_map(|p| match p.into_opt() {
                    PointOpt::Uv(uv) => Some(uv_to_volt(uv)),
                    // TODO: Support separation
                    PointOpt::Sep => None,
                }))
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
                .write(uv_to_volt(self.input.load(Ordering::Acquire)))
                .await;
        }
    }
}

impl AdcHandle {
    pub async fn push_iter<I: ExactSizeIterator<Item = Point>>(&mut self, points: I) {
        let mut last = Uv::default();
        let len = points.len();
        self.buffer.wait_vacant(len).await;
        assert_eq!(
            self.buffer.push_iter(points.map(|p| {
                if let PointOpt::Uv(uv) = p.into_opt() {
                    last = uv;
                }
                p
            })),
            len
        );
        if len > 0 {
            self.last_point.store(last, Ordering::Release);
        }
    }
}
