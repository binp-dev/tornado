extern crate std;

use crate::Error;
use common::config;
use core::time::Duration;
use derive_more::{Deref, DerefMut};
use flatty::Portable;
use flatty_io::details;
use flatty_io::{Reader as InnerReader, Writer as InnerWriter};
use std::net::TcpStream;
use timeout_readwrite::{TimeoutReader, TimeoutWriter};
use ustd::task::Task;

const IP_ADDR: &str = "localhost:4578";

pub struct Channel(TcpStream);
impl Channel {
    pub fn new(task: &Task) -> Result<Self, Error> {
        Ok(Self(TcpStream::connect(IP_ADDR)?))
    }
    pub fn split(self) -> (ReadChannel, WriteChannel) {
        (self.0.try_clone().unwrap(), self.0)
    }
}

pub type ReadChannel = TcpStream;
pub type WriteChannel = TcpStream;

#[derive(Deref, DerefMut)]
pub struct Reader<M: Portable + ?Sized> {
    inner: InnerReader<M, TimeoutReader<ReadChannel>>,
}

#[derive(Deref, DerefMut)]
pub struct Writer<M: Portable + ?Sized> {
    inner: InnerWriter<M, TimeoutWriter<WriteChannel>>,
}

impl<M: Portable + ?Sized> Reader<M> {
    pub fn new(channel: ReadChannel, timeout: Option<Duration>) -> Self {
        Self {
            inner: InnerReader::new(TimeoutReader::new(channel, timeout), config::MAX_MCU_MSG_LEN),
        }
    }
}

impl<M: Portable + ?Sized> Writer<M> {
    pub fn new(channel: WriteChannel, timeout: Option<Duration>) -> Self {
        Self {
            inner: InnerWriter::new(TimeoutWriter::new(channel, timeout), config::MAX_APP_MSG_LEN),
        }
    }
}

pub type ReadGuard<'a, M> = details::ReadGuard<'a, M, ReadChannel>;
pub type UninitWriteGuard<'a, M> = details::UninitWriteGuard<'a, M, TimeoutReader<ReadChannel>>;
pub type WriteGuard<'a, M> = details::WriteGuard<'a, M, TimeoutWriter<WriteChannel>>;
