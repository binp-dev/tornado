use super::Error;
use crate::{
    epics,
    utils::double_vec::{self, DoubleVec},
};
use async_std::task::spawn;
use common::config::{volt_to_dac, Point};
use ferrite::{atomic::AtomicVariable, typed::Type, TypedVariable as Variable};
use futures::{
    future::join_all,
    stream::{self, Stream},
};
use std::sync::Arc;

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
                state: Box::new(unfold_variable(epics.state, |x| x != 0)),
                mode: Box::new(unfold_variable(epics.mode, |x| x != 0)),
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
    pub state: Box<dyn Stream<Item = bool> + Send>,
    pub mode: Box<dyn Stream<Item = bool> + Send>,
}

fn unfold_variable<T: Send, V: Type, F: Fn(V) -> T>(
    var: Variable<V>,
    map: F,
) -> impl Stream<Item = T> {
    stream::unfold((var, map), move |(mut var, map)| async move {
        Some((map(var.wait().await.read().await), (var, map)))
    })
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
