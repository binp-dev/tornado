use super::Error;
use crate::{
    epics,
    utils::double_vec::{self, DoubleVec},
};
use async_atomic::AsyncAtomic;
use async_std::task::spawn;
use common::config::{volt_to_dac, Point};
use ferrite::{atomic::AtomicVariable, TypedVariable as Variable};
use flatty_io::AsyncWriter as MsgWriter;
use futures::{future::join_all, select, FutureExt};
use std::{future::Future, sync::Arc};

pub struct Dac {
    epics: epics::Dac,
    buffer: double_vec::Writer<Point>,
    request: Arc<AtomicVariable<u16>>,
}

impl Dac {
    pub fn new(epics: epics::Dac) -> (Self, DacHandle) {
        let buffer = DoubleVec::<Point>::new(epics.array.max_len());
        let (read_buffer, write_buffer) = buffer.split();

        let request = AtomicVariable::<u16>::new(epics.request);

        (
            Self {
                buffer: write_buffer,
                epics,
                request: request.clone(),
            },
            DacHandle {
                buffer: read_buffer,
                read_ready: Box::new(move || request.store(1)),
            },
        )
    }

    pub fn run(self) -> impl Future<Output = Result<(), Error>> {
        let array_reader = ArrayReader {
            input: self.epics.array,
            output: self.buffer.clone(),
            request: self.request.clone(),
        };
        let scalar_reader = ScalarReader {
            input: self.epics.scalar,
            output: self.buffer,
            request: self.request,
        };
        let state_reader = StateReader {
            state: self.epics.state,
            mode: self.epics.mode,
        };

        async move {
            join_all([
                spawn(array_reader.run()),
                spawn(scalar_reader.run()),
                spawn(state_reader.run()),
            ])
            .await;
            Ok(())
        }
    }
}

pub struct DacHandle {
    pub buffer: double_vec::Reader<Point>,
    pub read_ready: Box<dyn FnMut()>,
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

struct VariableReader<T: Copy + From<U>, U> {
    variable: Variable<U>,
    value: Arc<AsyncAtomic<T>>,
}

impl<T: Copy + From<U>, U> VariableReader<T, U> {
    async fn run(mut self) {
        loop {
            self.value.store(self.state.wait().await.read().await);
        }
    }
}
