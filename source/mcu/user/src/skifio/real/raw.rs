use crate::{
    hal::{RetCode, Timeout},
    skifio::*,
};
use core::ffi::c_void;

pub type RawDiCallback = extern "C" fn(*mut c_void, Di);

extern "C" {
    pub fn skifio_init() -> RetCode;
    pub fn skifio_deinit() -> RetCode;

    pub fn skifio_ao_enable() -> RetCode;
    pub fn skifio_ao_disable() -> RetCode;

    pub fn skifio_transfer(out: *const XferOut, in_: *mut XferIn) -> RetCode;
    pub fn skifio_wait_ready(timeout: Timeout) -> RetCode;

    pub fn skifio_do_write(value: Do) -> RetCode;

    pub fn skifio_di_read() -> Di;
    pub fn skifio_di_subscribe(callback: *mut RawDiCallback, data: *mut c_void) -> RetCode;
    pub fn skifio_di_unsubscribe() -> RetCode;
}
