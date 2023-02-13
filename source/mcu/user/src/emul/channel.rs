extern crate std;

use common::config;
use core::time::Duration;
use derive_more::{Deref, DerefMut};
use flatty::Portable;
use flatty_io::{Reader as InnerReader, Writer as InnerWriter};
use timeout_readwrite::{TimeoutReader, TimeoutWriter};

pub use flatty_io::details as inner;
pub use std::net::TcpStream as Channel;

pub type ReadChannel = Channel;
pub type WriteChannel = Channel;

#[derive(Deref, DerefMut)]
pub struct Reader<M: Portable + ?Sized> {
    inner: InnerReader<M, TimeoutReader<Channel>>,
}

#[derive(Deref, DerefMut)]
pub struct Writer<M: Portable + ?Sized> {
    inner: InnerWriter<M, TimeoutWriter<Channel>>,
}

impl<M: Portable + ?Sized> Reader<M> {
    pub fn new(channel: ReadChannel, timeout: Option<Duration>) -> Self {
        Self {
            inner: InnerReader::new(
                TimeoutReader::new(channel, timeout),
                config::MAX_MCU_MSG_LEN,
            ),
        }
    }
}

impl<M: Portable + ?Sized> Writer<M> {
    pub fn new(channel: WriteChannel, timeout: Option<Duration>) -> Self {
        Self {
            inner: InnerWriter::new(
                TimeoutWriter::new(channel, timeout),
                config::MAX_APP_MSG_LEN,
            ),
        }
    }
}

pub type ReadGuard<'a, M> = inner::ReadGuard<'a, M, TimeoutReader<Channel>>;
pub type UninitWriteGuard<'a, M> = inner::UninitWriteGuard<'a, M, TimeoutWriter<Channel>>;
pub type WriteGuard<'a, M> = inner::WriteGuard<'a, M, TimeoutWriter<Channel>>;
