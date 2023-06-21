use super::Error;
use crate::{
    epics,
    utils::double_vec::{self, DoubleVec},
};
use async_atomic::GenericSubscriber;
use common::values::{volt_to_uv_saturating, Uv};
use ferrite::{atomic::AtomicVariable, TypedVariable as Variable};
use futures::{future::join_all, Stream, StreamExt};
use std::{pin::Pin, sync::Arc};
use tokio::task::spawn;

pub struct Dac {
    array: ArrayReader,
    scalar: ScalarReader,
}

impl Dac {
    pub fn new(epics: epics::Dac) -> (Self, DacHandle) {
        let buffer = DoubleVec::<Uv>::new(epics.array.max_len());
        let (read_buffer, write_buffer) = buffer.split();

        let request = AtomicVariable::new(epics.request);
        request.store(1);
        let mode = AtomicVariable::new(epics.mode);
        let addition = GenericSubscriber::new(AtomicVariable::new(epics.addition));

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
                addition: Box::pin(addition.into_stream().map(volt_to_uv_saturating)),
                state: Box::pin(epics.state.into_stream().map(|x| x != 0)),
            },
        )
    }

    pub async fn run(self) -> Result<(), Error> {
        join_all([spawn(self.array.run()), spawn(self.scalar.run())]).await;
        Ok(())
    }
}

pub struct DacHandle {
    pub buffer: double_vec::ReadIterator<Uv, DacModifier>,
    pub addition: Pin<Box<dyn Stream<Item = Uv> + Send>>,
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
    output: Arc<double_vec::Writer<Uv>>,
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
                output.extend(input.iter().copied().map(volt_to_uv_saturating));
            }
            input.accept().await;
        }
    }
}

struct ScalarReader {
    input: Variable<f64>,
    output: Arc<double_vec::Writer<Uv>>,
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
                output.push(volt_to_uv_saturating(value));
            }
        }
    }
}
