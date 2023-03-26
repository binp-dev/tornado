use super::Error;
use common::values::{Din as DinValue, Dout as DoutValue, Value};
use ferrite::TypedVariable as Variable;
use futures::{
    channel::mpsc::{channel, Receiver, Sender},
    SinkExt, StreamExt,
};

const DOUT_BUFFER_SIZE: usize = 8;
const DIN_BUFFER_SIZE: usize = 64;

pub struct Dout {
    variable: Variable<u32>,
    channel: Sender<DoutValue>,
}

pub type DoutHandle = Receiver<DoutValue>;

impl Dout {
    pub fn new(epics: Variable<u32>) -> (Self, DoutHandle) {
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
            let value = DoutValue::try_from_base(
                self.variable.wait().await.read().await.try_into().unwrap(),
            )
            .unwrap();
            if self.channel.send(value).await.is_err() {
                break Err(Error::Disconnected);
            }
        }
    }
}

pub struct Din {
    variable: Variable<u32>,
    channel: Receiver<DinValue>,
}

pub type DinHandle = Sender<DinValue>;

impl Din {
    pub fn new(epics: Variable<u32>) -> (Self, DinHandle) {
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
                .write(value.into_base() as u32)
                .await;
        }
    }
}
