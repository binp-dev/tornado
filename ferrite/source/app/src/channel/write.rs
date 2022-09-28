use async_std::{
    io::{self, Write, WriteExt},
    sync::{Arc, Mutex},
};
use flatty::{self, mem::MaybeUninitUnsized, prelude::*};
use std::{
    marker::PhantomData,
    ops::{Deref, DerefMut},
};

// Writer

pub struct MsgWriter<M: Portable + ?Sized, W: Write + Unpin> {
    writer: Arc<Mutex<W>>,
    buffer: Vec<u8>,
    _phantom: PhantomData<M>,
}

impl<M: Portable + ?Sized, W: Write + Unpin> MsgWriter<M, W> {
    pub fn new(writer: W, max_msg_size: usize) -> Self {
        Self {
            writer: Arc::new(Mutex::new(writer)),
            buffer: vec![0; max_msg_size],
            _phantom: PhantomData,
        }
    }

    pub fn new_uninit_msg(&mut self) -> MsgUninitWriteGuard<M, W> {
        MsgUninitWriteGuard { owner: self }
    }
}

impl<M: Portable + FlatDefault + ?Sized, W: Write + Unpin> MsgWriter<M, W> {
    pub fn init_default_msg(&mut self) -> Result<MsgWriteGuard<M, W>, flatty::Error> {
        self.new_uninit_msg().default()
    }
}

impl<M: Portable + ?Sized, W: Write + Unpin> Clone for MsgWriter<M, W> {
    fn clone(&self) -> Self {
        Self {
            writer: self.writer.clone(),
            buffer: Vec::with_capacity(self.buffer.capacity()),
            _phantom: PhantomData,
        }
    }
}

// WriteGuard

pub struct MsgUninitWriteGuard<'a, M: Portable + ?Sized, W: Write + Unpin> {
    owner: &'a mut MsgWriter<M, W>,
}
pub struct MsgWriteGuard<'a, M: Portable + ?Sized, W: Write + Unpin> {
    owner: &'a mut MsgWriter<M, W>,
}

impl<'a, M: Portable + ?Sized, W: Write + Unpin> Unpin for MsgUninitWriteGuard<'a, M, W> {}
impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> Unpin for MsgWriteGuard<'a, M, W> {}

impl<'a, M: Portable + ?Sized, W: Write + Unpin> MsgUninitWriteGuard<'a, M, W> {
    /// # Safety
    ///
    /// Underlying message data must be initialized.
    pub unsafe fn assume_init(self) -> MsgWriteGuard<'a, M, W> {
        MsgWriteGuard { owner: self.owner }
    }
    pub fn validate(self) -> Result<MsgWriteGuard<'a, M, W>, flatty::Error> {
        M::validate(self.deref())?;
        Ok(unsafe { self.assume_init() })
    }
}
impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> MsgUninitWriteGuard<'a, M, W> {
    pub fn default(self) -> Result<MsgWriteGuard<'a, M, W>, flatty::Error> {
        M::placement_default(&mut self.owner.buffer)?;
        Ok(unsafe { self.assume_init() })
    }
}
impl<'a, M: Portable + FlatDefault + ?Sized, W: Write + Unpin> MsgWriteGuard<'a, M, W> {
    pub async fn write(self) -> Result<(), io::Error> {
        let mut guard = self.owner.writer.lock().await;
        guard.write_all(&self.owner.buffer[..self.size()]).await
    }
}

impl<'a, M: Portable + ?Sized, W: Write + Unpin> Deref for MsgUninitWriteGuard<'a, M, W> {
    type Target = MaybeUninitUnsized<M>;
    fn deref(&self) -> &Self::Target {
        MaybeUninitUnsized::from_bytes(&self.owner.buffer).unwrap()
    }
}
impl<'a, M: Portable + ?Sized, W: Write + Unpin> DerefMut for MsgUninitWriteGuard<'a, M, W> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        MaybeUninitUnsized::from_mut_bytes(&mut self.owner.buffer).unwrap()
    }
}

impl<'a, M: Portable + ?Sized, W: Write + Unpin> Deref for MsgWriteGuard<'a, M, W> {
    type Target = M;
    fn deref(&self) -> &M {
        unsafe { MaybeUninitUnsized::from_bytes_unchecked(&self.owner.buffer).assume_init_ref() }
    }
}
impl<'a, M: Portable + ?Sized, W: Write + Unpin> DerefMut for MsgWriteGuard<'a, M, W> {
    fn deref_mut(&mut self) -> &mut M {
        unsafe { MaybeUninitUnsized::from_mut_bytes_unchecked(&mut self.owner.buffer).assume_init_mut() }
    }
}
