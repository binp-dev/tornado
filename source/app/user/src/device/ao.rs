use super::Error;
use crate::{
    epics,
    utils::double_vec::{self, DoubleVec},
};
use async_atomic::GenericSubscriber;
use common::values::{volt_to_uv_saturating, Uv};
use ferrite::{atomic::AtomicVariable, TypedVariable as Variable};
use futures::{Stream, StreamExt};
use std::{pin::Pin, sync::Arc};

pub struct Ao {
    next: NextReader,
}

impl Ao {
    pub fn new(epics: epics::Ao) -> (Self, AoHandle) {
        let buffer = DoubleVec::<Uv>::new(epics.next_waveform.max_len());
        let (read_buffer, write_buffer) = buffer.split();

        let ready = AtomicVariable::new(epics.next_ready);
        ready.store(1);
        let cycle = AtomicVariable::new(epics.next_cycle);
        let add = GenericSubscriber::new(AtomicVariable::new(epics.add));

        (
            Self {
                next: NextReader {
                    input: epics.next_waveform,
                    output: write_buffer,
                    ready: ready.clone(),
                },
            },
            AoHandle {
                buffer: read_buffer.into_iter(AoModifier { ready, cycle }),
                add: Box::pin(add.into_stream().map(volt_to_uv_saturating)),
            },
        )
    }

    pub async fn run(self) -> Result<(), Error> {
        self.next.run().await;
        Ok(())
    }
}

pub struct AoHandle {
    pub buffer: double_vec::ReadIterator<Uv, AoModifier>,
    // TODO: Remove `Box` when `impl Trait` stabilized.
    pub add: Pin<Box<dyn Stream<Item = Uv> + Send>>,
}

pub struct AoModifier {
    ready: Arc<AtomicVariable<u16>>,
    cycle: Arc<AtomicVariable<u16>>,
}

impl double_vec::ReadModifier for AoModifier {
    fn cyclic(&self) -> bool {
        self.cycle.load() == 0
    }
    fn swap(&mut self) {
        self.ready.store(1)
    }
}

struct NextReader {
    input: Variable<[f64]>,
    output: Arc<double_vec::Writer<Uv>>,
    ready: Arc<AtomicVariable<u16>>,
}

impl NextReader {
    async fn run(mut self) {
        loop {
            let input = self.input.wait().await;
            self.ready.store(0);
            {
                let mut output = self.output.write().await;
                output.clear();
                output.extend(input.iter().copied().map(volt_to_uv_saturating));
            }
            input.accept().await;
        }
    }
}
