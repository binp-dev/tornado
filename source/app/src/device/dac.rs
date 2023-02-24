use crate::{channel::Channel, epics};
use common::{
    config::{volt_to_dac, Point, PointPortable},
    protocol::{self as proto, AppMsg, AppMsgMut},
};
use ferrite::{
    misc::{
        double_vec::{self, DoubleVec},
        AsyncCounter,
    },
    variable::{atomic::AtomicVariableU16, ArrayVariable, Variable},
    VarSync,
};
use flatty::{flat_vec, portable::NativeCast};
use flatty_io::AsyncWriter as MsgWriter;
use futures::{executor::ThreadPool, join, select, FutureExt};
use std::future::Future;
use std::sync::Arc;

pub struct Dac<C: Channel> {
    pub channel: MsgWriter<AppMsg, C::Write>,
    pub epics: epics::Dac,
}

impl<C: Channel> Dac<C> {
    pub fn run(self, exec: Arc<ThreadPool>) -> (impl Future<Output = ()>, DacHandle) {
        let buffer = DoubleVec::<Point>::new(self.epics.array.max_len());
        let (read_buffer, write_buffer) = buffer.split();
        let point_counter = Arc::new(AsyncCounter::new(0));

        let handle = DacHandle {
            points_to_send: point_counter.clone(),
        };

        let request = AtomicVariableU16::new(self.epics.request, &exec).unwrap();

        let array_reader = ArrayReader {
            input: self.epics.array,
            output: write_buffer.clone(),
            request: request.clone(),
        };
        let scalar_reader = ScalarReader {
            input: self.epics.scalar,
            output: write_buffer,
            request: request.clone(),
        };
        let state_reader = StateReader::<C> {
            state: self.epics.state,
            mode: self.epics.mode,
            channel: self.channel.clone(),
        };

        let msg_sender = PointSender::<C> {
            channel: self.channel,
            stream: BufferReadStream::new(read_buffer, request),
            points_to_send: point_counter,
        };

        (
            async move {
                exec.spawn_ok(array_reader.run());
                exec.spawn_ok(scalar_reader.run());
                exec.spawn_ok(state_reader.run());
                exec.spawn_ok(msg_sender.run());
            },
            handle,
        )
    }
}

pub struct DacHandle {
    points_to_send: Arc<AsyncCounter>,
}

impl DacHandle {
    pub fn request(&self, count: usize) {
        self.points_to_send.add(count);
    }
}

struct ArrayReader {
    input: ArrayVariable<f64>,
    output: Arc<double_vec::Writer<i32>>,
    request: Arc<AtomicVariableU16>,
}

impl ArrayReader {
    async fn run(mut self) {
        loop {
            let input = self.input.acquire().await;
            self.request.write(0);
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
    output: Arc<double_vec::Writer<i32>>,
    request: Arc<AtomicVariableU16>,
}

impl ScalarReader {
    async fn run(mut self) {
        loop {
            let value = self.input.acquire().await.read().await;
            self.request.write(0);
            {
                let mut output = self.output.write().await;
                output.clear();
                output.push(value as i32);
            }
        }
    }
}

struct StateReader<C: Channel> {
    state: Variable<u16>,
    mode: Variable<u16>,
    channel: MsgWriter<AppMsg, C::Write>,
}

impl<C: Channel> StateReader<C> {
    async fn send_state(&mut self, state: bool) {
        self.channel
            .new_message()
            .emplace(proto::AppMsgInitDacState {
                enable: state as u8,
            })
            .unwrap()
            .write()
            .await
            .unwrap();
    }

    async fn run(mut self) {
        loop {
            select! {
                state = async { self.state.acquire().await.read().await }.fuse() => {
                    self.send_state(state != 0).await;
                },
                _mode = async { self.mode.acquire().await.read().await }.fuse() => {
                    unimplemented!()
                },
            }
        }
    }
}

struct PointSender<C: Channel> {
    channel: MsgWriter<AppMsg, C::Write>,
    stream: BufferReadStream<i32>,
    points_to_send: Arc<AsyncCounter>,
}

impl<C: Channel> PointSender<C> {
    async fn run(mut self) {
        self.stream.request.write(1);
        loop {
            join!(self.points_to_send.wait(1), async {
                if self.stream.is_empty() {
                    self.stream.wait_ready().await;
                }
            });

            let mut msg = self
                .channel
                .new_message()
                .emplace(proto::AppMsgInitDacData {
                    points: flat_vec![],
                })
                .unwrap();
            let will_send = if let AppMsgMut::DacData { points } = msg.as_mut() {
                let mut count = self.points_to_send.sub(None);
                //log::debug!("points_to_send: {}", count);
                while count > 0 && !points.is_full() {
                    match self.stream.next().await {
                        Some(value) => {
                            points.push([PointPortable::from_native(value)]).unwrap();
                            count -= 1;
                        }
                        None => break,
                    }
                }
                //log::debug!("points_send: {}", msg.points.len());
                self.points_to_send.add(count);
                !points.is_empty()
            } else {
                unreachable!();
            };
            if will_send {
                msg.write().await.unwrap();
            }
        }
    }
}

struct BufferReadStream<T: Clone> {
    buffer: double_vec::Reader<T>,
    pos: usize,
    cyclic: bool,
    request: Arc<AtomicVariableU16>,
}
impl<T: Clone> BufferReadStream<T> {
    pub fn new(buffer: double_vec::Reader<T>, request: Arc<AtomicVariableU16>) -> Self {
        BufferReadStream {
            buffer,
            pos: 0,
            cyclic: false,
            request,
        }
    }
    pub async fn try_swap(&mut self) -> bool {
        //log::info!("try swap");
        if self.buffer.try_swap().await {
            self.request.write(1);
            true
        } else {
            false
        }
    }
    pub async fn next(&mut self) -> Option<T> {
        loop {
            if self.pos < self.buffer.len() {
                let value = self.buffer[self.pos].clone();
                self.pos += 1;
                break Some(value);
            } else if self.try_swap().await || self.cyclic {
                self.pos = 0;
            } else {
                break None;
            }
        }
    }
    pub async fn wait_ready(&mut self) {
        self.buffer.wait_ready().await
    }
    pub fn len(&self) -> usize {
        self.buffer.len() - self.pos
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}
