use core::ffi::c_char;

#[repr(transparent)]
pub struct Timeout(pub u32);

impl Timeout {
    const NON_BLOCK: u32 = 0;
    const WAIT_FOREVER: u32 = 0xFFFFFFFF;
}

#[repr(u8)]
pub enum RetCode {
    /// Success
    Success = 0x00,
    /// Generic failure
    Failure = 0x01,
    /// Memory allocation failure
    BadAlloc = 0x02,
    /// Try to access element out of container bounds
    OutOfBounds = 0x03,
    /// User provided invalid input
    InvalidInput = 0x04,
    /// Invalid data generated during process
    InvalidData = 0x05,

    /// Functionality isn't implemented yet
    Unimplemented = 0xFE,
    /// Timeout exceeded
    TimedOut = 0xFF,
}

extern "C" {
    fn hal_retcode_str(code: RetCode) -> *const c_char;
}
