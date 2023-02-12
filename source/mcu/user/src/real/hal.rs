use core::{ffi::c_char, time::Duration};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
#[repr(transparent)]
pub struct Timeout(pub u32);

impl Timeout {
    pub const NON_BLOCK: Timeout = Timeout(0);
    pub const WAIT_FOREVER: Timeout = Timeout(0xFFFFFFFF);
}

impl From<Option<Duration>> for Timeout {
    fn from(src: Option<Duration>) -> Timeout {
        match src {
            Some(dur) => Timeout(dur.as_millis() as u32),
            None => Timeout::WAIT_FOREVER,
        }
    }
}

extern "C" {
    fn hal_retcode_str(code: RetCode) -> *const c_char;
}
