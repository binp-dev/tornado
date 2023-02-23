use crate::{
    hal::{RetCode, Timeout},
    skifio::*,
};
use core::ffi::c_void;

pub type RawDinCallback = extern "C" fn(*mut c_void, Din);

extern "C" {
    pub fn skifio_init() -> RetCode;
    pub fn skifio_deinit() -> RetCode;

    pub fn skifio_dac_enable() -> RetCode;
    pub fn skifio_dac_disable() -> RetCode;

    pub fn skifio_transfer(out: *const XferOut, in_: *mut XferIn) -> RetCode;
    pub fn skifio_wait_ready(timeout: Timeout) -> RetCode;

    pub fn skifio_dout_write(value: Dout) -> RetCode;

    pub fn skifio_din_read() -> Din;
    pub fn skifio_din_subscribe(callback: *mut RawDinCallback, data: *mut c_void) -> RetCode;
    pub fn skifio_din_unsubscribe() -> RetCode;
}
