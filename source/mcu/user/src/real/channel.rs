extern crate alloc;

use super::hal::{RetCode, Timeout};
use crate::Error;
use alloc::{
    alloc::{alloc, dealloc, Layout},
    sync::Arc,
};
use core::{
    marker::PhantomData,
    mem,
    ops::{Deref, DerefMut},
    ptr, slice,
    time::Duration,
};
use flatty::{mem::MaybeUninitUnsized, Emplacer, FlatDefault, Portable};

#[repr(C)]
struct HalRpmsgChannel {
    _unused: [u8; 0],
}

extern "C" {
    static __hal_rpmsg_channel_size: usize;
    static __hal_rpmsg_channel_align: usize;

    fn hal_rpmsg_init();

    fn hal_rpmsg_deinit();

    fn hal_rpmsg_create_channel(channel: *mut HalRpmsgChannel, remote_id: u32) -> RetCode;

    fn hal_rpmsg_destroy_channel(channel: *mut HalRpmsgChannel) -> RetCode;

    fn hal_rpmsg_alloc_tx_buffer(
        channel: *mut HalRpmsgChannel,
        tx_buf: *mut *mut u8,
        size: *mut usize,
        timeout: Timeout,
    ) -> RetCode;

    fn hal_rpmsg_free_rx_buffer(channel: *mut HalRpmsgChannel, rx_buf: *mut u8) -> RetCode;

    fn hal_rpmsg_send_nocopy(channel: *mut HalRpmsgChannel, tx_buf: *mut u8, len: usize)
        -> RetCode;

    fn hal_rpmsg_recv_nocopy(
        channel: *mut HalRpmsgChannel,
        rx_buf: *mut *mut u8,
        len: *mut usize,
        timeout: Timeout,
    ) -> RetCode;
}

impl HalRpmsgChannel {
    fn layout() -> Layout {
        unsafe { Layout::from_size_align(__hal_rpmsg_channel_size, __hal_rpmsg_channel_align) }
            .unwrap()
    }

    unsafe fn alloc() -> Option<*mut Self> {
        let this = alloc(Self::layout()) as *mut Self;
        if !this.is_null() {
            Some(this)
        } else {
            None
        }
    }
    unsafe fn dealloc(this: *mut Self) {
        dealloc(this as *mut u8, Self::layout())
    }
}

pub struct Channel {
    raw: *mut HalRpmsgChannel,
}

pub struct ReadChannel(Arc<Channel>);
pub struct WriteChannel(Arc<Channel>);

impl Channel {
    pub fn new(remote_id: u32) -> Result<Self, Error> {
        let raw = unsafe { HalRpmsgChannel::alloc() }.ok_or(Error::Alloc)?;

        match unsafe { hal_rpmsg_create_channel(raw, remote_id) } {
            RetCode::Success => (),
            r => {
                unsafe { HalRpmsgChannel::dealloc(raw) };
                return Err(Error::Hal(r));
            }
        }

        Ok(Self { raw })
    }

    pub fn split(self) -> (ReadChannel, WriteChannel) {
        let this = Arc::new(self);
        (ReadChannel(this.clone()), WriteChannel(this))
    }
}

impl Drop for Channel {
    fn drop(&mut self) {
        unsafe {
            hal_rpmsg_destroy_channel(self.raw);
            HalRpmsgChannel::dealloc(self.raw);
        }
    }
}

struct ReadBuffer<'a> {
    channel: &'a Channel,
    ptr: *mut u8,
    len: usize,
}
impl<'a> Deref for ReadBuffer<'a> {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.ptr as *const _, self.len) }
    }
}

struct WriteBuffer<'a> {
    channel: &'a Channel,
    ptr: *mut u8,
    len: usize,
}
impl<'a> Deref for WriteBuffer<'a> {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.ptr as *const _, self.len) }
    }
}
impl<'a> DerefMut for WriteBuffer<'a> {
    fn deref_mut(&mut self) -> &mut [u8] {
        unsafe { slice::from_raw_parts_mut(self.ptr, self.len) }
    }
}

impl ReadChannel {
    fn recv(&mut self, timeout: Option<Duration>) -> Result<ReadBuffer, Error> {
        let mut buf = ReadBuffer {
            channel: &self.0,
            ptr: ptr::null_mut(),
            len: 0,
        };
        match unsafe {
            hal_rpmsg_recv_nocopy(
                self.0.raw,
                &mut buf.ptr as *mut _,
                &mut buf.len as *mut _,
                timeout.into(),
            )
        } {
            RetCode::Success => (),
            r => return Err(Error::Hal(r)),
        }
        Ok(buf)
    }
}
impl<'a> Drop for ReadBuffer<'a> {
    fn drop(&mut self) {
        assert_eq!(
            unsafe { hal_rpmsg_free_rx_buffer(self.channel.raw, self.ptr) },
            RetCode::Success
        );
    }
}

impl WriteChannel {
    fn alloc(&self, timeout: Option<Duration>) -> Result<WriteBuffer<'_>, Error> {
        let mut buf = WriteBuffer {
            channel: &self.0,
            ptr: ptr::null_mut(),
            len: 0,
        };
        match unsafe {
            hal_rpmsg_alloc_tx_buffer(
                self.0.raw,
                &mut buf.ptr as *mut _,
                &mut buf.len as *mut _,
                timeout.into(),
            )
        } {
            RetCode::Success => (),
            r => return Err(Error::Hal(r)),
        }
        Ok(buf)
    }
}
impl<'a> WriteBuffer<'a> {
    fn send(self, len: usize) -> Result<(), Error> {
        assert!(len <= self.len);
        let res = match unsafe { hal_rpmsg_send_nocopy(self.channel.raw, self.ptr, len) } {
            RetCode::Success => Ok(()),
            r => Err(Error::Hal(r)),
        };
        mem::forget(self);
        res
    }
}
impl<'a> Drop for WriteBuffer<'a> {
    fn drop(&mut self) {
        panic!("WriteBuffer must be sent manually");
    }
}

pub struct Reader<M: Portable + ?Sized> {
    channel: ReadChannel,
    _p: PhantomData<M>,
}

pub struct Writer<M: Portable + ?Sized> {
    channel: WriteChannel,
    _p: PhantomData<M>,
}

pub struct ReadGuard<'a, M: Portable + ?Sized> {
    buffer: ReadBuffer<'a>,
    _p: PhantomData<M>,
}

impl<M: Portable + ?Sized> Reader<M> {
    pub fn new(channel: ReadChannel) -> Self {
        Self {
            channel,
            _p: PhantomData,
        }
    }

    pub fn read_message(&mut self, timeout: Option<Duration>) -> Result<ReadGuard<'_, M>, Error> {
        let buffer = self.channel.recv(timeout)?;
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
    pub fn new(channel: WriteChannel) -> Self {
        Self {
            channel,
            _p: PhantomData,
        }
    }

    pub fn new_message(
        &mut self,
        timeout: Option<Duration>,
    ) -> Result<UninitWriteGuard<'_, M>, Error> {
        let mut buffer = self.channel.alloc(timeout)?;
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
