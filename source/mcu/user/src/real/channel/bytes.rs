extern crate alloc;

use super::raw::{self, HalRpmsgChannel};
use crate::{hal::RetCode, println, Error};
use alloc::sync::Arc;
use core::{
    mem,
    ops::{Deref, DerefMut},
    ptr, slice,
    time::Duration,
};
use freertos::{Duration as FreeRtosDuration, Mutex, Task};
use lazy_static::lazy_static;

lazy_static! {
    static ref RPMSG: Mutex<GlobalRpmsg> = Mutex::new(GlobalRpmsg::new()).unwrap();
}

struct GlobalRpmsg {
    uses: usize,
}
impl GlobalRpmsg {
    fn new() -> Self {
        Self { uses: 0 }
    }
    fn acquire(&mut self, _task: &Task) {
        if self.uses == 0 {
            unsafe { raw::hal_rpmsg_init() };
            println!("RPMsg subsystem initialized");
        }
        self.uses += 1;
    }
    fn release(&mut self) {
        self.uses -= 1;
        if self.uses == 0 {
            unsafe { raw::hal_rpmsg_deinit() };
            println!("RPMsg subsystem deinitialized");
        }
    }
}
impl Drop for GlobalRpmsg {
    fn drop(&mut self) {}
}

pub struct Channel {
    raw: *mut HalRpmsgChannel,
}

unsafe impl Send for Channel {}
unsafe impl Sync for Channel {}

pub struct ReadChannel(Arc<Channel>);
pub struct WriteChannel(Arc<Channel>);

impl Channel {
    pub fn new(task: &Task, remote_id: u32) -> Result<Self, Error> {
        RPMSG
            .lock(FreeRtosDuration::infinite())
            .unwrap()
            .acquire(task);

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
            RPMSG.lock(FreeRtosDuration::infinite()).unwrap().release();
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
            r => {
                buf.ptr = ptr::null_mut();
                return Err(Error::Hal(r));
            }
        }
        Ok(buf)
    }
}
impl<'a> Drop for ReadBuffer<'a> {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            assert_eq!(
                unsafe { raw::hal_rpmsg_free_rx_buffer(self.channel.raw, self.ptr) },
                RetCode::Success
            );
        }
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
            r => {
                mem::forget(buf);
                return Err(Error::Hal(r));
            }
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
