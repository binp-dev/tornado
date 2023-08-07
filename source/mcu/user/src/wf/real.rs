use core::{
    slice,
    sync::atomic::{fence, Ordering},
};
use derive_more::{Deref, DerefMut};

extern "C" {
    static wf_max_offset: usize;

    static wf_offset_align: usize;

    fn wf_addr(offset: usize) -> *mut u8;

    fn wf_acquire(addr: *mut u8, len: usize);
    fn wf_release(addr: *mut u8, len: usize);
}

#[derive(Deref)]
pub struct WfRead {
    data: &'static [u8],
}

#[derive(Deref, DerefMut)]
pub struct WfWrite {
    data: &'static mut [u8],
}

macro_rules! assert_align {
    ($value:expr) => {
        assert!($value % unsafe { wf_offset_align } == 0, "Misalinged Wf offset");
    };
}

pub unsafe fn read(offset: usize, len: usize) -> WfRead {
    assert_align!(offset);
    assert_align!(len);
    assert!(offset + len < unsafe { wf_max_offset });

    let addr = unsafe { wf_addr(offset) };
    unsafe { wf_acquire(addr, len) };
    let ret = WfRead {
        data: slice::from_raw_parts(addr, len),
    };
    fence(Ordering::Acquire);
    ret
}

pub unsafe fn write(offset: usize, len: usize) -> WfWrite {
    assert_align!(offset);
    assert_align!(len);
    assert!(offset + len < unsafe { wf_max_offset });

    unsafe {
        WfWrite {
            data: slice::from_raw_parts_mut(wf_addr(offset), len),
        }
    }
}

impl Drop for WfWrite {
    fn drop(&mut self) {
        fence(Ordering::Release);
        unsafe { wf_release(self.as_mut_ptr(), self.len()) };
    }
}
