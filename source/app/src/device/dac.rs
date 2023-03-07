use super::Error;
use crate::{
    epics,
    utils::{
        double_vec::{self, DoubleVec},
        misc::unfold_variable,
    },
};
use async_std::task::spawn;
use common::config::{volt_to_dac, Point};
use ferrite::{atomic::AtomicVariable, TypedVariable as Variable};
use futures::{future::join_all, Stream};
use std::{pin::Pin, sync::Arc};

pub struct Dac {
    array: ArrayReader,
    scalar: ScalarReader,
}

impl Dac {
    pub fn new(epics: epics::Dac) -> (Self, DacHandle) {
        let buffer = DoubleVec::<Point>::new(epics.array.max_len());
        let (read_buffer, write_buffer) = buffer.split();

        let request = AtomicVariable::<u16>::new(epics.request);

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
                buffer: read_buffer,
                read_ready: Box::new(move || request.store(1)),
                state: Box::pin(unfold_variable(epics.state, |x| Some(x != 0))),
                mode: Box::pin(unfold_variable(epics.mode, |x| Some(x != 0))),
            },
        )
    }

    pub async fn run(self) -> Result<(), Error> {
        join_all([spawn(self.array.run()), spawn(self.scalar.run())]).await;
        Ok(())
    }
}

// TODO: Remove `Box`es when `impl Trait` stabilized.
pub struct DacHandle {
    pub buffer: double_vec::Reader<Point>,
    pub read_ready: Box<dyn FnMut() + Send>,
    pub state: Pin<Box<dyn Stream<Item = bool> + Send>>,
    pub mode: Pin<Box<dyn Stream<Item = bool> + Send>>,
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
                output.extend(input.iter().copied().map(volt_to_dac));
                log::debug!("array_read: len={}", input.len());
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
                output.push(value as Point);
            }
        }
    }
}
