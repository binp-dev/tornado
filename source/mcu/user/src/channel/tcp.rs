extern crate std;

use crate::Error;
use common::config;
use core::time::Duration;
use derive_more::{Deref, DerefMut};
use flatty::Portable;
use flatty_io::{details, Reader as InnerReader, Writer as InnerWriter};
use std::{
    io,
    net::{TcpListener, TcpStream},
};
use timeout_readwrite::{TimeoutReader, TimeoutWriter};
use ustd::task::Task;

pub struct Channel(TcpStream);
impl Channel {
    pub fn new(_task: &Task) -> Result<Self, Error> {
        let lis = TcpListener::bind(config::CHANNEL_ADDR)?;
        let stream = lis.incoming().next().unwrap()?;
        Ok(Self(stream))
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
    pub fn new_message(&mut self) -> Result<UninitWriteGuard<'_, M>, io::Error> {
        Ok(self.inner.new_message())
    }
}

pub type ReadGuard<'a, M> = details::ReadGuard<'a, M, TimeoutReader<ReadChannel>>;
pub type UninitWriteGuard<'a, M> = details::UninitWriteGuard<'a, M, TimeoutWriter<WriteChannel>>;
pub type WriteGuard<'a, M> = details::WriteGuard<'a, M, TimeoutWriter<WriteChannel>>;
