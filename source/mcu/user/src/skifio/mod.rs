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
    config::ADC_COUNT,
    units::{AdcPoint, DacPoint},
};
use core::{sync::atomic::AtomicU8, time::Duration};
use ustd::interrupt::InterruptContext;

pub const DIN_SIZE: usize = 8;
pub const DOUT_SIZE: usize = 4;

pub type Din = u8;
pub type Dout = u8;

#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferIn {
    pub adcs: [AdcPoint; ADC_COUNT],
}
#[repr(C)]
#[derive(Clone, Debug, Default)]
pub struct XferOut {
    pub dac: DacPoint,
}

pub type AtomicDin = AtomicU8;
pub type AtomicDout = AtomicU8;

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
