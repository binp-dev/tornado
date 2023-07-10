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
    values::{Di, Do, Uv},
};
use core::time::Duration;
use ustd::task::InterruptContext;

#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferIn {
    pub ais: [Uv; AI_COUNT],
    pub temp: i8,
    pub status: u8,
}

#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferOut {
    pub ao: Uv,
}

pub trait DiHandler: FnMut(&mut InterruptContext, Di) + Send + 'static {}
impl<T: FnMut(&mut InterruptContext, Di) + Send + 'static> DiHandler for T {}

pub trait SkifioIface: Send + Sync {
    fn set_ao_state(&mut self, enabled: bool) -> Result<(), Error>;
    fn ao_state(&self) -> bool;

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error>;
    fn transfer(&mut self, out: XferOut) -> Result<XferIn, Error>;

    fn write_do(&mut self, do_: Do) -> Result<(), Error>;

    fn read_di(&mut self) -> Di;
    fn subscribe_di(&mut self, callback: Option<Box<dyn DiHandler>>) -> Result<(), Error>;
}
