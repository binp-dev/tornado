#[cfg(feature = "emul")]
extern crate std;

#[cfg(feature = "real")]
use crate::hal;
use derive_more::From;
#[cfg(feature = "real")]
use freertos::FreeRtosError;
#[cfg(feature = "emul")]
use std::io;

#[derive(Debug, From)]
pub enum Error {
    Alloc,
    #[cfg(feature = "real")]
    FreeRtos(FreeRtosError),
    #[cfg(feature = "real")]
    #[from(ignore)]
    Hal(hal::RetCode),
    #[cfg(feature = "emul")]
    Io(io::Error),
    Flatty(flatty::Error),
    Other(&'static str),
}

impl From<hal::RetCode> for Result<(), Error> {
    fn from(value: hal::RetCode) -> Self {
        if value == hal::RetCode::Success {
            Ok(())
        } else {
            Err(Error::Hal(value))
        }
    }
}
