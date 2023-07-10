use super::Error;
use crate::epics;
use async_ringbuf::{traits::*, AsyncHeapRb};
use common::values::{uv_to_volt, AtomicUv, Point, PointOpt, Uv};
use ferrite::TypedVariable as Variable;
use ringbuf::traits::*;
use std::{
    iter::ExactSizeIterator,
    sync::{atomic::Ordering, Arc},
};

pub struct Ai {
    input: <AsyncHeapRb<Point> as Split>::Cons,
    output: Variable<[f64]>,
}

pub struct AiHandle {
    buffer: <AsyncHeapRb<Point> as Split>::Prod,
    last_point: Arc<AtomicUv>,
}

impl Ai {
    pub fn new(epics: epics::Ai) -> (Self, AiHandle) {
        let buffer = AsyncHeapRb::<Point>::new(2 * epics.waveform.max_len());
        let (producer, consumer) = buffer.split();
        let last = Arc::new(AtomicUv::default());
        (
            Self {
                input: consumer,
                output: epics.waveform,
            },
            AiHandle {
                buffer: producer,
                last_point: last,
            },
        )
    }

    pub async fn run(mut self) -> Result<(), Error> {
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

impl AiHandle {
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
