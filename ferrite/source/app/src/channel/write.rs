use async_std::io::{self, Write, WriteExt};
use flatty::{self, prelude::*};
use std::{
    marker::PhantomData,
    ops::{Deref, DerefMut},
};

// Writer

pub struct MsgWriter<M: Portable + FlatDefault + ?Sized, W: Write + Unpin> {
    writer: W,
    buffer: Vec<u8>,
    _phantom: PhantomData<M>,
}

impl<M: Portable + FlatDefault + ?Sized, W: Write + Unpin> MsgWriter<M, W> {
    pub fn new(writer: W, max_msg_size: usize) -> Self {
        Self {
            writer,
            buffer: vec![0; max_msg_size],
            _phantom: PhantomData,
        }
    }

    pub fn init_default_msg(&mut self) -> Result<MsgWriteGuard<M, W>, flatty::Error> {
        M::placement_default(&mut self.buffer)?;
        Ok(MsgWriteGuard { owner: self })
    }
}

// WriteGuard

pub struct MsgWriteGuard<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> {
    owner: &'a mut MsgWriter<M, W>,
}

impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> Unpin for MsgWriteGuard<'a, M, W> {}

impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> MsgWriteGuard<'a, M, W> {
    pub async fn write(self) -> Result<(), io::Error> {
        self.owner.writer.write_all(&self.owner.buffer[..self.size()]).await
    }
}

impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> Deref for MsgWriteGuard<'a, M, W> {
    type Target = M;
    fn deref(&self) -> &M {
        M::from_bytes(&self.owner.buffer).unwrap()
    }
}
impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> DerefMut for MsgWriteGuard<'a, M, W> {
    fn deref_mut(&mut self) -> &mut M {
        M::from_mut_bytes(&mut self.owner.buffer).unwrap()
    }
}
