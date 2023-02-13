extern crate alloc;

use crate::hal::{RetCode, Timeout};
use alloc::alloc::{alloc, dealloc, Layout};

#[repr(C)]
pub struct HalRpmsgChannel {
    _unused: [u8; 0],
}

extern "C" {
    pub static __hal_rpmsg_channel_size: usize;
    pub static __hal_rpmsg_channel_align: usize;

    pub fn hal_rpmsg_init();

    pub fn hal_rpmsg_deinit();

    pub fn hal_rpmsg_create_channel(channel: *mut HalRpmsgChannel, remote_id: u32) -> RetCode;

    pub fn hal_rpmsg_destroy_channel(channel: *mut HalRpmsgChannel) -> RetCode;

    pub fn hal_rpmsg_alloc_tx_buffer(
        channel: *mut HalRpmsgChannel,
        tx_buf: *mut *mut u8,
        size: *mut usize,
        timeout: Timeout,
    ) -> RetCode;

    pub fn hal_rpmsg_free_rx_buffer(channel: *mut HalRpmsgChannel, rx_buf: *mut u8) -> RetCode;

    pub fn hal_rpmsg_send_nocopy(
        channel: *mut HalRpmsgChannel,
        tx_buf: *mut u8,
        len: usize,
    ) -> RetCode;

    pub fn hal_rpmsg_recv_nocopy(
        channel: *mut HalRpmsgChannel,
        rx_buf: *mut *mut u8,
        len: *mut usize,
        timeout: Timeout,
    ) -> RetCode;
}

impl HalRpmsgChannel {
    pub fn layout() -> Layout {
        unsafe { Layout::from_size_align(__hal_rpmsg_channel_size, __hal_rpmsg_channel_align) }
            .unwrap()
    }

    pub unsafe fn alloc() -> Option<*mut Self> {
        let this = alloc(Self::layout()) as *mut Self;
        if !this.is_null() {
            Some(this)
        } else {
            None
        }
    }
    pub unsafe fn dealloc(this: *mut Self) {
        dealloc(this as *mut u8, Self::layout())
    }
}
