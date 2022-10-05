use crate::{
    channel::Channel,
    config::Point,
    epics,
    proto::{AppMsg, AppMsgMut, AppMsgTag},
};
use async_std::sync::Arc;
use ferrite::{
    channel::MsgWriter,
    misc::{
        double_vec::{self, DoubleVec},
        AsyncCounter, AsyncFlag,
    },
    variable::{ReadArrayVariable, ReadVariable},
    WriteVariable,
};
use flatty::portable::{le::I32, NativeCast};
use futures::join;
use std::future::Future;

pub struct Dac {
    pub channel: MsgWriter<AppMsg, Channel>,
    pub epics: epics::Dac,
}

impl Dac {
    pub fn run(self) -> (impl Future<Output = ()>, DacHandle) {
        let buffer = DoubleVec::<Point>::new(self.epics.array.max_len());
        let (read_buffer, write_buffer) = buffer.split();
        let point_counter = Arc::new(AsyncCounter::new(0));

        let handle = DacHandle {
            points_to_send: point_counter.clone(),
        };

        let requester = Requester::new(self.epics.request);

        let array_reader = ArrayReader {
            input: self.epics.array,
            output: write_buffer.clone(),
            request: requester.flag.clone(),
        };
        let scalar_reader = ScalarReader {
            input: self.epics.scalar,
            output: write_buffer,
            request: requester.flag.clone(),
        };

        let msg_sender = MsgSender {
            channel: self.channel,
            stream: BufferReadStream::new(read_buffer, requester.flag.clone()),
            points_to_send: point_counter,
        };

        (
            async move {
                join!(array_reader.run(), scalar_reader.run(), msg_sender.run(), requester.run());
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
        log::debug!("request: {}", count);
        self.points_to_send.add(count);
    }
}

struct ArrayReader {
    input: ReadArrayVariable<f64>,
    output: Arc<double_vec::Writer<i32>>,
    request: Arc<AsyncFlag>,
}

impl ArrayReader {
    async fn run(mut self) {
        loop {
            let input = self.input.read_in_place().await;
            {
                let mut output = self.output.write().await;
                output.clear();
                output.extend(input.iter().map(|x| *x as i32));
                log::debug!("array_read: len={}", input.len());
            }
            self.request.take();
        }
    }
}

struct ScalarReader {
    input: ReadVariable<i32>,
    output: Arc<double_vec::Writer<i32>>,
    request: Arc<AsyncFlag>,
}

impl ScalarReader {
    async fn run(mut self) {
        loop {
            let value = self.input.read().await;
            {
                let mut output = self.output.write().await;
                output.clear();
                output.push(value as i32);
                log::debug!("scalar_read: {}", value);
            }
            self.request.take();
        }
    }
}

struct MsgSender {
    channel: MsgWriter<AppMsg, Channel>,
    stream: BufferReadStream<i32>,
    points_to_send: Arc<AsyncCounter>,
}

impl MsgSender {
    async fn run(mut self) {
        loop {
            log::debug!("dac waiting");
            join!(self.points_to_send.wait(1), async {
                if self.stream.is_empty() {
                    self.stream.wait_ready().await;
                    log::debug!("buffer ready");
                }
            });

            let mut msg = self.channel.init_default_msg().unwrap();
            msg.reset_tag(AppMsgTag::DacData).unwrap();
            let will_send = if let AppMsgMut::DacData(msg) = msg.as_mut() {
                let mut count = self.points_to_send.sub(None);
                log::debug!("points_to_send: {}", count);
                while count > 0 && !msg.points.is_full() {
                    match self.stream.next().await {
                        Some(value) => {
                            msg.points.push(I32::from_native(value)).unwrap();
                            count -= 1;
                        }
                        None => break,
                    }
                }
                log::debug!("points_send: {}", msg.points.len());
                self.points_to_send.add(count);
                !msg.points.is_empty()
            } else {
                unreachable!();
            };
            log::debug!("write msg");
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
    request: Arc<AsyncFlag>,
}
impl<T: Clone> BufferReadStream<T> {
    pub fn new(buffer: double_vec::Reader<T>, request: Arc<AsyncFlag>) -> Self {
        BufferReadStream {
            buffer,
            pos: 0,
            cyclic: false,
            request,
        }
    }
    pub async fn try_swap(&mut self) -> bool {
        log::info!("try swap");
        if self.buffer.try_swap().await {
            self.request.set();
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
    pub async fn wait_ready(&self) {
        self.buffer.wait_ready().await
    }
    pub fn len(&self) -> usize {
        self.buffer.len() - self.pos
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

struct Requester {
    var: WriteVariable<u32>,
    flag: Arc<AsyncFlag>,
}

impl Requester {
    fn new(var: WriteVariable<u32>) -> Self {
        Self {
            var,
            flag: Arc::new(AsyncFlag::new(true)),
        }
    }
    async fn run(mut self) {
        self.var.write(1).await;
        loop {
            self.flag.wait().await;
            self.var.write(1).await;
        }
    }
}
