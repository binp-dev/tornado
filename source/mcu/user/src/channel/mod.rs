#[cfg(feature = "real")]
mod rpmsg;
#[cfg(feature = "real")]
pub use rpmsg::*;

#[cfg(feature = "emul")]
mod tcp;
#[cfg(feature = "emul")]
pub use tcp::*;
