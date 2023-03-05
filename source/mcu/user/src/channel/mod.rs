#[cfg(feature = "real")]
mod rpmsg;
#[cfg(feature = "real")]
pub use rpmsg::*;

#[cfg(feature = "fake")]
mod tcp;
#[cfg(feature = "fake")]
pub use tcp::*;
