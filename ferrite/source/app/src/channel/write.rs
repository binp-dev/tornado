use async_std::io::{self, Write, WriteExt};
use flatty::{self, Flat, Portable};
use std::{
    marker::PhantomData,
    ops::{Deref, DerefMut},
};

// Writer

pub struct MsgWriter<M: Flat + Portable + ?Sized, W: Write + Unpin> {
    writer: W,
    buffer: Vec<u8>,
    _phantom: PhantomData<M>,
}

impl<M: Flat + Portable + ?Sized, W: Write + Unpin> MsgWriter<M, W> {
    pub fn new(writer: W, max_msg_size: usize) -> Self {
        Self {
            writer,
            buffer: vec![0; max_msg_size],
            _phantom: PhantomData,
        }
    }

    pub fn init_msg(&mut self, init: &M::Dyn) -> Result<MsgWriteGuard<M, W>, flatty::Error> {
        M::placement_new(&mut self.buffer, init)?;
        Ok(MsgWriteGuard { owner: self })
    }
}

// WriteGuard

pub struct MsgWriteGuard<'a, M: Flat + Portable + ?Sized, W: Write + Unpin> {
    owner: &'a mut MsgWriter<M, W>,
}

impl<'a, M: Flat + Portable + ?Sized, W: Write + Unpin> Unpin for MsgWriteGuard<'a, M, W> {}

impl<'a, M: Flat + Portable + ?Sized, W: Write + Unpin> MsgWriteGuard<'a, M, W> {
    pub async fn write(self) -> Result<(), io::Error> {
        self.owner.writer.write_all(&self.owner.buffer[..self.size()]).await
    }
}

impl<'a, M: Flat + Portable + ?Sized, W: Write + Unpin> Deref for MsgWriteGuard<'a, M, W> {
    type Target = M;
    fn deref(&self) -> &M {
        M::reinterpret(&self.owner.buffer).unwrap()
    }
}
impl<'a, M: Flat + Portable + ?Sized, W: Write + Unpin> DerefMut for MsgWriteGuard<'a, M, W> {
    fn deref_mut(&mut self) -> &mut M {
        M::reinterpret_mut(&mut self.owner.buffer).unwrap()
    }
}
