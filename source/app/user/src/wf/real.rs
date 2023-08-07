use derive_more::{Deref, DerefMut};
use std::{
    fs::{File, OpenOptions},
    io,
    os::{
        fd::{FromRawFd, IntoRawFd, RawFd},
        raw::c_void,
    },
    path::Path,
    ptr, slice,
    sync::atomic::{fence, Ordering},
};

pub struct WfData {
    fd: RawFd,
    data: *mut u8,
    len: usize,
}

#[derive(Deref)]
pub struct WfRead<'a> {
    data: &'a [u8],
}

#[derive(Deref, DerefMut)]
pub struct WfWrite<'a> {
    data: &'a mut [u8],
}

unsafe impl Sync for WfData {}
unsafe impl Send for WfData {}

impl WfData {
    const LEN: usize = 0x10000;

    pub fn new(path: &Path) -> io::Result<Self> {
        let fd = {
            let file = OpenOptions::new().read(true).write(true).open(path)?;
            file.into_raw_fd()
        };
        let len = Self::LEN;
        let data = {
            let r = unsafe {
                libc::mmap(
                    ptr::null_mut(),
                    len,
                    libc::PROT_READ | libc::PROT_WRITE,
                    libc::MAP_SHARED,
                    fd,
                    0,
                )
            };
            if r == libc::MAP_FAILED {
                drop(unsafe { File::from_raw_fd(fd) });
                return Err(io::Error::last_os_error());
            }
            r as *mut u8
        };

        Ok(Self { fd, data, len })
    }
}

impl Drop for WfData {
    fn drop(&mut self) {
        unsafe { libc::munmap(self.data as *mut c_void, self.len) };
        unsafe { File::from_raw_fd(self.fd) };
    }
}

impl WfData {
    pub unsafe fn read(&self, offset: usize, len: usize) -> WfRead<'_> {
        assert!(offset + len <= self.len);
        fence(Ordering::Acquire);
        WfRead {
            data: slice::from_raw_parts(self.data.add(offset), len),
        }
    }

    pub unsafe fn write(&self, offset: usize, len: usize) -> WfWrite<'_> {
        assert!(offset + len <= Self::LEN);

        WfWrite {
            data: slice::from_raw_parts_mut(self.data.add(offset), len),
        }
    }
}

impl<'a> Drop for WfWrite<'a> {
    fn drop(&mut self) {
        fence(Ordering::Release);
    }
}
