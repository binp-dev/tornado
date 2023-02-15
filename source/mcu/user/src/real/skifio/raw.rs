use crate::hal::{RetCode, Timeout};
use common::config::ADC_COUNT;
use core::ffi::c_void;
#[cfg(feature = "skifio-debug")]
use core::sync::atomic::AtomicU64;

pub const DIN_SIZE: usize = 8;
pub const DOUT_SIZE: usize = 4;

pub type Ain = i32;
pub type Aout = i16;
pub type Din = u8;
pub type Dout = u8;
pub type DinCallback = extern "C" fn(*mut c_void, Din);

#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferIn {
    pub adcs: [Ain; ADC_COUNT],
}
#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferOut {
    pub dac: Aout,
}
#[repr(C)]
#[cfg(feature = "skifio-debug")]
struct DebugInfo {
    intr_count: AtomicU64,
}

extern "C" {
    pub fn skifio_init() -> RetCode;
    pub fn skifio_deinit() -> RetCode;

    pub fn skifio_dac_enable() -> RetCode;
    pub fn skifio_dac_disable() -> RetCode;

    pub fn skifio_transfer(out: *const XferOut, in_: *mut XferIn) -> RetCode;
    pub fn skifio_wait_ready(timeout: Timeout) -> RetCode;

    pub fn skifio_dout_write(value: Dout) -> RetCode;

    pub fn skifio_din_read() -> Din;
    pub fn skifio_din_subscribe(callback: *mut DinCallback, data: *mut c_void) -> RetCode;
    pub fn skifio_din_unsubscribe() -> RetCode;

    #[cfg(feature = "skifio-debug")]
    pub static _SKIFIO_DEBUG_INFO: DebugInfo;
}
