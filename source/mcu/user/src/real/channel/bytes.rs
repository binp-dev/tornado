extern crate alloc;

use super::raw::{self, HalRpmsgChannel};
use crate::{hal::RetCode, Error};
use alloc::sync::Arc;
use core::{
    mem,
    ops::{Deref, DerefMut},
    ptr, slice,
    time::Duration,
};

pub struct Channel {
    raw: *mut HalRpmsgChannel,
}

pub struct ReadChannel(Arc<Channel>);
pub struct WriteChannel(Arc<Channel>);

impl Channel {
    pub fn new(remote_id: u32) -> Result<Self, Error> {
        let raw = unsafe { HalRpmsgChannel::alloc() }.ok_or(Error::Alloc)?;

        match unsafe { raw::hal_rpmsg_create_channel(raw, remote_id) } {
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
            raw::hal_rpmsg_destroy_channel(self.raw);
            HalRpmsgChannel::dealloc(self.raw);
        }
    }
}

pub struct ReadBuffer<'a> {
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

pub struct WriteBuffer<'a> {
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
    pub fn recv(&mut self, timeout: Option<Duration>) -> Result<ReadBuffer, Error> {
        let mut buf = ReadBuffer {
            channel: &self.0,
            ptr: ptr::null_mut(),
            len: 0,
        };
        match unsafe {
            raw::hal_rpmsg_recv_nocopy(
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
            unsafe { raw::hal_rpmsg_free_rx_buffer(self.channel.raw, self.ptr) },
            RetCode::Success
        );
    }
}

impl WriteChannel {
    pub fn alloc(&self, timeout: Option<Duration>) -> Result<WriteBuffer<'_>, Error> {
        let mut buf = WriteBuffer {
            channel: &self.0,
            ptr: ptr::null_mut(),
            len: 0,
        };
        match unsafe {
            raw::hal_rpmsg_alloc_tx_buffer(
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
    pub fn send(self, len: usize) -> Result<(), Error> {
        assert!(len <= self.len);
        let res = match unsafe { raw::hal_rpmsg_send_nocopy(self.channel.raw, self.ptr, len) } {
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
