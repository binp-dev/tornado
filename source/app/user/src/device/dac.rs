use super::Error;
use crate::{
    epics,
    utils::{
        double_vec::{self, DoubleVec},
        misc::unfold_variable,
    },
};
use common::values::Point;
use ferrite::{atomic::AtomicVariable, TypedVariable as Variable};
use futures::{future::join_all, Stream};
use std::{pin::Pin, sync::Arc};
use tokio::task::spawn;

pub struct Dac {
    array: ArrayReader,
    scalar: ScalarReader,
}

impl Dac {
    pub fn new(epics: epics::Dac) -> (Self, DacHandle) {
        let buffer = DoubleVec::<Point>::new(epics.array.max_len());
        let (read_buffer, write_buffer) = buffer.split();

        let request = AtomicVariable::new(epics.request);
        request.store(1);
        let mode = AtomicVariable::new(epics.mode);

        (
            Self {
                array: ArrayReader {
                    input: epics.array,
                    output: write_buffer.clone(),
                    request: request.clone(),
                },
                scalar: ScalarReader {
                    input: epics.scalar,
                    output: write_buffer,
                    request: request.clone(),
                },
            },
            DacHandle {
                buffer: read_buffer.into_iter(DacModifier { request, mode }),
                state: Box::pin(unfold_variable(epics.state, |x| Some(x != 0))),
            },
        )
    }

    pub async fn run(self) -> Result<(), Error> {
        join_all([spawn(self.array.run()), spawn(self.scalar.run())]).await;
        Ok(())
    }
}

pub struct DacHandle {
    pub buffer: double_vec::ReadIterator<Point, DacModifier>,
    // TODO: Remove `Box` when `impl Trait` stabilized.
    pub state: Pin<Box<dyn Stream<Item = bool> + Send>>,
}

pub struct DacModifier {
    request: Arc<AtomicVariable<u16>>,
    mode: Arc<AtomicVariable<u16>>,
}

impl double_vec::ReadModifier for DacModifier {
    fn cyclic(&self) -> bool {
        self.mode.load() != 0
    }
    fn swap(&mut self) {
        self.request.store(1)
    }
}

struct ArrayReader {
    input: Variable<[f64]>,
    output: Arc<double_vec::Writer<Point>>,
    request: Arc<AtomicVariable<u16>>,
}

impl ArrayReader {
    async fn run(mut self) {
        loop {
            let input = self.input.wait().await;
            self.request.store(0);
            {
                let mut output = self.output.write().await;
                output.clear();
                output.extend(input.iter().copied().map(Point::from_analog_saturating));
            }
            input.accept().await;
        }
    }
}

struct ScalarReader {
    input: Variable<f64>,
    output: Arc<double_vec::Writer<Point>>,
    request: Arc<AtomicVariable<u16>>,
}

impl ScalarReader {
    async fn run(mut self) {
        loop {
            let value = self.input.wait().await.read().await;
            self.request.store(0);
            {
                let mut output = self.output.write().await;
                output.clear();
                output.push(Point::from_analog_saturating(value));
            }
        }
    }
}
