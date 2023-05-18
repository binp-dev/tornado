extern crate alloc;

use super::wrapper::{ReadBuffer, ReadChannel, WriteBuffer, WriteChannel};
use crate::Error;
use core::{
    marker::PhantomData,
    ops::{Deref, DerefMut},
    time::Duration,
};
use flatty::{Emplacer, Flat, FlatDefault};

pub struct Reader<M: Flat + ?Sized> {
    channel: ReadChannel,
    timeout: Option<Duration>,
    _p: PhantomData<M>,
}

pub struct Writer<M: Flat + ?Sized> {
    channel: WriteChannel,
    timeout: Option<Duration>,
    _p: PhantomData<M>,
}

pub struct ReadGuard<'a, M: Flat + ?Sized> {
    buffer: ReadBuffer<'a>,
    _p: PhantomData<M>,
}

impl<M: Flat + ?Sized> Reader<M> {
    pub fn new(channel: ReadChannel, timeout: Option<Duration>) -> Self {
        Self {
            channel,
            timeout,
            _p: PhantomData,
        }
    }

    pub fn read_message(&mut self) -> Result<ReadGuard<'_, M>, Error> {
        let buffer = self.channel.recv(self.timeout)?;
        assert_eq!(buffer.as_ptr().align_offset(M::ALIGN), 0);
        M::from_bytes(&buffer)?;
        Ok(ReadGuard { buffer, _p: PhantomData })
    }
}

impl<'a, M: Flat + ?Sized> Deref for ReadGuard<'a, M> {
    type Target = M;
    fn deref(&self) -> &M {
        unsafe { M::from_bytes_unchecked(&self.buffer) }
    }
}

impl<M: Flat + ?Sized> Writer<M> {
    pub fn new(channel: WriteChannel, timeout: Option<Duration>) -> Self {
        Self {
            channel,
            timeout,
            _p: PhantomData,
        }
    }

    pub fn alloc_message(&mut self) -> Result<UninitWriteGuard<'_, M>, Error> {
        let buffer = self.channel.alloc(self.timeout)?;
        assert_eq!(buffer.as_ptr().align_offset(M::ALIGN), 0);
        Ok(UninitWriteGuard { buffer, _p: PhantomData })
    }
}

pub struct UninitWriteGuard<'a, M: Flat + ?Sized> {
    buffer: WriteBuffer<'a>,
    _p: PhantomData<M>,
}

pub struct WriteGuard<'a, M: Flat + ?Sized> {
    buffer: WriteBuffer<'a>,
    _p: PhantomData<M>,
}
impl<'a, M: Flat + ?Sized> Deref for WriteGuard<'a, M> {
    type Target = M;
    fn deref(&self) -> &Self::Target {
        unsafe { M::from_bytes_unchecked(&self.buffer) }
    }
}
impl<'a, M: Flat + ?Sized> DerefMut for WriteGuard<'a, M> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { M::from_mut_bytes_unchecked(&mut self.buffer) }
    }
}

impl<'a, M: Flat + ?Sized> UninitWriteGuard<'a, M> {
    /// # Safety
    ///
    /// Underlying message data must be initialized.
    pub unsafe fn assume_init(self) -> WriteGuard<'a, M> {
        WriteGuard {
            buffer: self.buffer,
            _p: PhantomData,
        }
    }

    pub fn new_in_place(mut self, emplacer: impl Emplacer<M>) -> Result<WriteGuard<'a, M>, Error> {
        M::new_in_place(&mut self.buffer, emplacer)?;
        Ok(WriteGuard {
            buffer: self.buffer,
            _p: PhantomData,
        })
    }
}

impl<'a, M: Flat + FlatDefault + ?Sized> UninitWriteGuard<'a, M> {
    pub fn default_in_place(self) -> Result<WriteGuard<'a, M>, Error> {
        self.new_in_place(M::default_emplacer())
    }
}

impl<'a, M: Flat + ?Sized> WriteGuard<'a, M> {
    pub fn write(self) -> Result<(), Error> {
        let size = self.size();
        self.buffer.send(size)
    }
}
