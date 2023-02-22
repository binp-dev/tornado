use common::config::ADC_COUNT;
use core::sync::atomic::AtomicU8;
use ustd::interrupt::InterruptContext;

pub const DIN_SIZE: usize = 8;
pub const DOUT_SIZE: usize = 4;

pub type Ain = i32;
pub type Aout = i16;
pub type Din = u8;
pub type Dout = u8;

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

pub type AtomicDin = AtomicU8;
pub type AtomicDout = AtomicU8;

pub trait DinHandler: FnMut(&mut InterruptContext, Din) + Send + 'static {}
impl<T: FnMut(&mut InterruptContext, Din) + Send + 'static> DinHandler for T {}
