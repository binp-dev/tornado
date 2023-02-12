use super::hal::{RetCode, Timeout};

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
