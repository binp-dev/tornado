use super::Error;
use common::values::{Di as DiValue, Do as DoValue};
use ferrite::TypedVariable as Variable;
use futures::{
    channel::mpsc::{channel, Receiver, Sender},
    SinkExt, StreamExt,
};

const DOUT_BUFFER_SIZE: usize = 8;
const DIN_BUFFER_SIZE: usize = 64;

pub struct Do {
    variable: Variable<u32>,
    channel: Sender<DoValue>,
}

pub type DoHandle = Receiver<DoValue>;

impl Do {
    pub fn new(epics: Variable<u32>) -> (Self, DoHandle) {
        let (sender, receiver) = channel(DOUT_BUFFER_SIZE);
        (
            Self {
                variable: epics,
                channel: sender,
            },
            receiver,
        )
    }
    pub async fn run(mut self) -> Result<(), Error> {
        loop {
            let input = u8::try_from(self.variable.wait().await.read().await).unwrap();
            let value = DoValue::try_from(input).unwrap();
            if self.channel.send(value).await.is_err() {
                break Err(Error::Disconnected);
            }
        }
    }
}

pub struct Di {
    variable: Variable<u32>,
    channel: Receiver<DiValue>,
}

pub type DiHandle = Sender<DiValue>;

impl Di {
    pub fn new(epics: Variable<u32>) -> (Self, DiHandle) {
        let (sender, reciever) = channel(DIN_BUFFER_SIZE);
        (
            Self {
                variable: epics,
                channel: reciever,
            },
            sender,
        )
    }
    pub async fn run(mut self) -> Result<(), Error> {
        loop {
            let value = match self.channel.next().await {
                Some(value) => value,
                None => break Err(Error::Disconnected),
            };
            self.variable
                .request()
                .await
                .write(u8::from(value) as u32)
                .await;
        }
    }
}
