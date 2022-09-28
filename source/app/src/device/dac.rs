use crate::{
    channel::Channel,
    config::Point,
    epics,
    proto::{AppMsg, AppMsgMut, AppMsgTag},
};
use async_std::sync::{Arc, Mutex};
use ferrite::{
    channel::MsgWriter,
    misc::{
        double_vec::{self, DoubleVec},
        AsyncCounter,
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
        let request = Arc::new(Mutex::new(self.epics.request));

        let handle = DacHandle {
            points_to_send: point_counter.clone(),
        };

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

        let msg_sender = MsgSender {
            channel: self.channel,
            stream: read_buffer.into_stream(),
            points_to_send: point_counter,
            request,
        };

        (
            async move {
                join!(array_reader.run(), scalar_reader.run(), msg_sender.run());
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
        println!("[app] request: {}", count);
        self.points_to_send.add(count);
    }
}

struct ArrayReader {
    input: ReadArrayVariable<f64>,
    output: Arc<double_vec::Writer<i32>>,
    request: Arc<Mutex<WriteVariable<u32>>>,
}

impl ArrayReader {
    async fn run(mut self) {
        loop {
            let input = self.input.read_in_place().await;
            {
                let mut output = self.output.write().await;
                output.clear();
                output.extend(input.iter().map(|x| *x as i32));
                println!("[app] array_read: len={}", input.len());
            }
            self.request.lock().await.write(0).await;
        }
    }
}

struct ScalarReader {
    input: ReadVariable<i32>,
    output: Arc<double_vec::Writer<i32>>,
    request: Arc<Mutex<WriteVariable<u32>>>,
}

impl ScalarReader {
    async fn run(mut self) {
        loop {
            let value = self.input.read().await;
            {
                let mut output = self.output.write().await;
                output.clear();
                output.push(value as i32);
                println!("[app] scalar_read: {}", value);
            }
            self.request.lock().await.write(0).await;
        }
    }
}

struct MsgSender {
    channel: MsgWriter<AppMsg, Channel>,
    stream: double_vec::ReadStream<i32>,
    points_to_send: Arc<AsyncCounter>,
    request: Arc<Mutex<WriteVariable<u32>>>,
}

impl MsgSender {
    async fn run(mut self) {
        loop {
            if !self.stream.buffer().ready() {
                println!("[app] dac request");
                self.request.lock().await.write(1).await;
            }

            join!(self.points_to_send.wait(1), async {
                if self.stream.buffer().is_empty() {
                    self.stream.buffer().wait_ready().await;
                }
            });

            let mut msg = self.channel.init_default_msg().unwrap();
            msg.reset_tag(AppMsgTag::DacData).unwrap();
            if let AppMsgMut::DacData(msg) = msg.as_mut() {
                let mut count = self.points_to_send.sub(None);
                while count > 0 && !msg.points.is_full() {
                    match self.stream.next().await {
                        Some(value) => {
                            msg.points.push(I32::from_native(value)).unwrap();
                            count -= 1;
                        }
                        None => break,
                    }
                }
                println!("[app] points_send: {}", msg.points.len());
                self.points_to_send.add(count);
            } else {
                unreachable!();
            }
            msg.write().await.unwrap();
        }
    }
}
