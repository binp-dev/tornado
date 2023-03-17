#[cfg(feature = "fake")]
extern crate std;

#[cfg(feature = "real")]
use crate::hal;
use derive_more::From;
#[cfg(feature = "fake")]
use std::io;
#[cfg(feature = "real")]
use ustd::freertos::FreeRtosError;

#[derive(Debug)]
pub enum ErrorKind {
    TimedOut,
    InvalidInput,
    InvalidData,
    BadAlloc,
    Other,
}

#[derive(Debug, From)]
pub enum ErrorSource {
    #[cfg(feature = "real")]
    Hal(hal::RetCode),
    #[cfg(feature = "real")]
    FreeRtos(FreeRtosError),
    #[cfg(feature = "fake")]
    Io(io::Error),
    Flatty(flatty::Error),
    #[from(ignore)]
    Other(&'static str),
    None,
}

#[derive(Debug)]
pub struct Error {
    pub kind: ErrorKind,
    pub source: ErrorSource,
}

#[cfg(feature = "real")]
impl From<hal::RetCode> for Result<(), Error> {
    fn from(code: hal::RetCode) -> Self {
        match code {
            hal::RetCode::Success => Ok(()),
            hal::RetCode::OutOfBounds => Err(ErrorKind::Other),
            hal::RetCode::Failure => Err(ErrorKind::Other),
            hal::RetCode::BadAlloc => Err(ErrorKind::BadAlloc),
            hal::RetCode::InvalidInput => Err(ErrorKind::InvalidInput),
            hal::RetCode::InvalidData => Err(ErrorKind::InvalidData),
            hal::RetCode::Unimplemented => Err(ErrorKind::Other),
            hal::RetCode::TimedOut => Err(ErrorKind::TimedOut),
        }
        .map_err(|kind| Error {
            kind,
            source: code.into(),
        })
    }
}

#[cfg(feature = "real")]
impl From<FreeRtosError> for Error {
    fn from(err: FreeRtosError) -> Self {
        let kind = match err {
            FreeRtosError::OutOfMemory => ErrorKind::BadAlloc,
            FreeRtosError::QueueSendTimeout => ErrorKind::TimedOut,
            FreeRtosError::QueueReceiveTimeout => ErrorKind::TimedOut,
            FreeRtosError::MutexTimeout => ErrorKind::TimedOut,
            FreeRtosError::Timeout => ErrorKind::TimedOut,
            FreeRtosError::QueueFull => ErrorKind::Other,
            FreeRtosError::StringConversionError => ErrorKind::InvalidInput,
            FreeRtosError::TaskNotFound => ErrorKind::InvalidInput,
            FreeRtosError::InvalidQueueSize => ErrorKind::InvalidInput,
            FreeRtosError::ProcessorHasShutDown => ErrorKind::Other,
        };
        Error {
            kind,
            source: err.into(),
        }
    }
}

#[cfg(feature = "fake")]
impl From<io::Error> for Error {
    fn from(err: io::Error) -> Self {
        let kind = match err.kind() {
            io::ErrorKind::OutOfMemory => ErrorKind::BadAlloc,
            io::ErrorKind::TimedOut => ErrorKind::TimedOut,
            io::ErrorKind::InvalidInput => ErrorKind::InvalidInput,
            io::ErrorKind::InvalidData => ErrorKind::InvalidData,
            _ => ErrorKind::Other,
        };
        Error {
            kind,
            source: err.into(),
        }
    }
}

impl From<flatty::Error> for Error {
    fn from(err: flatty::Error) -> Self {
        Error {
            kind: ErrorKind::InvalidData,
            source: err.into(),
        }
    }
}

#[cfg(feature = "fake")]
impl From<flatty_io::ReadError> for Error {
    fn from(err: flatty_io::ReadError) -> Self {
        match err {
            flatty_io::ReadError::Io(e) => e.into(),
            flatty_io::ReadError::Parse(e) => e.into(),
            flatty_io::ReadError::Eof => Error {
                kind: ErrorKind::InvalidData,
                source: ErrorSource::Other("EOF"),
            },
        }
    }
}
