extern crate alloc;

use super::bytes::{ReadBuffer, ReadChannel, WriteBuffer, WriteChannel};
use crate::Error;
use core::{
    marker::PhantomData,
    ops::{Deref, DerefMut},
    time::Duration,
};
use flatty::{mem::MaybeUninitUnsized, Emplacer, FlatDefault, Portable};

pub struct Reader<M: Portable + ?Sized> {
    channel: ReadChannel,
    timeout: Option<Duration>,
    _p: PhantomData<M>,
}

pub struct Writer<M: Portable + ?Sized> {
    channel: WriteChannel,
    timeout: Option<Duration>,
    _p: PhantomData<M>,
}

pub struct ReadGuard<'a, M: Portable + ?Sized> {
    buffer: ReadBuffer<'a>,
    _p: PhantomData<M>,
}

impl<M: Portable + ?Sized> Reader<M> {
    pub fn new(channel: ReadChannel, timeout: Option<Duration>) -> Self {
        Self {
            channel,
            timeout,
            _p: PhantomData,
        }
    }

    pub fn read_message(&mut self) -> Result<ReadGuard<'_, M>, Error> {
        let buffer = self.channel.recv(self.timeout)?;
        M::from_bytes(&buffer)?.validate()?;
        Ok(ReadGuard {
            buffer,
            _p: PhantomData,
        })
    }
}

impl<'a, M: Portable + ?Sized> Deref for ReadGuard<'a, M> {
    type Target = M;
    fn deref(&self) -> &M {
        unsafe { MaybeUninitUnsized::from_bytes_unchecked(&self.buffer).assume_init() }
    }
}

impl<M: Portable + ?Sized> Writer<M> {
    pub fn new(channel: WriteChannel, timeout: Option<Duration>) -> Self {
        Self {
            channel,
            timeout,
            _p: PhantomData,
        }
    }

    pub fn new_message(&mut self) -> Result<UninitWriteGuard<'_, M>, Error> {
        let mut buffer = self.channel.alloc(self.timeout)?;
        M::from_mut_bytes(&mut buffer)?;
        Ok(UninitWriteGuard {
            buffer,
            _p: PhantomData,
        })
    }
}

pub struct UninitWriteGuard<'a, M: Portable + ?Sized> {
    buffer: WriteBuffer<'a>,
    _p: PhantomData<M>,
}
impl<'a, M: Portable + ?Sized> Deref for UninitWriteGuard<'a, M> {
    type Target = MaybeUninitUnsized<M>;
    fn deref(&self) -> &Self::Target {
        unsafe { MaybeUninitUnsized::from_bytes_unchecked(&self.buffer) }
    }
}
impl<'a, M: Portable + ?Sized> DerefMut for UninitWriteGuard<'a, M> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { MaybeUninitUnsized::from_mut_bytes_unchecked(&mut self.buffer) }
    }
}

pub struct WriteGuard<'a, M: Portable + ?Sized> {
    buffer: WriteBuffer<'a>,
    _p: PhantomData<M>,
}
impl<'a, M: Portable + ?Sized> Deref for WriteGuard<'a, M> {
    type Target = M;
    fn deref(&self) -> &Self::Target {
        unsafe { MaybeUninitUnsized::from_bytes_unchecked(&self.buffer).assume_init() }
    }
}
impl<'a, M: Portable + ?Sized> DerefMut for WriteGuard<'a, M> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { MaybeUninitUnsized::from_mut_bytes_unchecked(&mut self.buffer).assume_init_mut() }
    }
}

impl<'a, M: Portable + ?Sized> UninitWriteGuard<'a, M> {
    /// # Safety
    ///
    /// Underlying message data must be initialized.
    pub unsafe fn assume_init(self) -> WriteGuard<'a, M> {
        WriteGuard {
            buffer: self.buffer,
            _p: PhantomData,
        }
    }

    pub fn emplace(mut self, emplacer: impl Emplacer<M>) -> Result<WriteGuard<'a, M>, Error> {
        M::new_in_place(&mut self, emplacer)?;
        Ok(WriteGuard {
            buffer: self.buffer,
            _p: PhantomData,
        })
    }
}

impl<'a, M: Portable + FlatDefault + ?Sized> UninitWriteGuard<'a, M> {
    pub fn default(self) -> Result<WriteGuard<'a, M>, Error> {
        self.emplace(M::default_emplacer())
    }
}

impl<'a, M: Portable + ?Sized> WriteGuard<'a, M> {
    pub fn write(self) -> Result<(), Error> {
        let size = self.size();
        self.buffer.send(size)
    }
}
