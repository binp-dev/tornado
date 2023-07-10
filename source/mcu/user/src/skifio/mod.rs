#[cfg(feature = "real")]
mod real;
#[cfg(feature = "real")]
pub use real::*;

#[cfg(feature = "fake")]
mod emul;
#[cfg(feature = "fake")]
pub use emul::*;

use crate::Error;
use alloc::boxed::Box;
use common::{
    config::AI_COUNT,
    values::{Din, Dout, Uv},
};
use core::time::Duration;
use ustd::task::InterruptContext;

#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferIn {
    pub adcs: [Uv; AI_COUNT],
    pub temp: i8,
    pub status: u8,
}

#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferOut {
    pub dac: Uv,
}

pub trait DinHandler: FnMut(&mut InterruptContext, Din) + Send + 'static {}
impl<T: FnMut(&mut InterruptContext, Din) + Send + 'static> DinHandler for T {}

pub trait SkifioIface: Send + Sync {
    fn set_dac_state(&mut self, enabled: bool) -> Result<(), Error>;
    fn dac_state(&self) -> bool;

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error>;
    fn transfer(&mut self, out: XferOut) -> Result<XferIn, Error>;

    fn write_dout(&mut self, dout: Dout) -> Result<(), Error>;

    fn read_din(&mut self) -> Din;
    fn subscribe_din(&mut self, callback: Option<Box<dyn DinHandler>>) -> Result<(), Error>;
}
