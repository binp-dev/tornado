use common::{config::ADC_COUNT, values::Point};
#[cfg(feature = "fake")]
use core::time::Duration;
use lazy_static::lazy_static;
use ringbuf::{CachedCons, CachedProd, StaticRb};
#[cfg(feature = "fake")]
use ringbuf_blocking::{BlockingCons, BlockingProd, BlockingRb};

#[cfg(feature = "real")]
pub type Rb<T, const N: usize> = StaticRb<T, N>;
#[cfg(feature = "real")]
pub type Producer<'a, T, const N: usize> = CachedProd<&'a Rb<T, N>>;
#[cfg(feature = "real")]
pub type Consumer<'a, T, const N: usize> = CachedCons<&'a Rb<T, N>>;

#[cfg(feature = "fake")]
pub type Rb<T, const N: usize> = BlockingRb<StaticRb<T, N>>;
#[cfg(feature = "fake")]
pub type Producer<'a, T, const N: usize> = BlockingProd<CachedProd<&'a Rb<T, N>>>;
#[cfg(feature = "fake")]
pub type Consumer<'a, T, const N: usize> = BlockingCons<CachedCons<&'a Rb<T, N>>>;

#[cfg(feature = "real")]
pub const DAC_BUFFER_LEN: usize = 1024;
#[cfg(feature = "real")]
pub const ADC_BUFFER_LEN: usize = 384;
#[cfg(feature = "fake")]
pub const DAC_BUFFER_LEN: usize = 16384;
#[cfg(feature = "fake")]
pub const ADC_BUFFER_LEN: usize = 16384;

#[cfg(feature = "fake")]
pub const BUFFER_TIMEOUT: Option<Duration> = Some(Duration::from_millis(1000));

pub type DacBuffer = Rb<Point, DAC_BUFFER_LEN>;
pub type AdcBuffer = Rb<[Point; ADC_COUNT], ADC_BUFFER_LEN>;

pub type DacProducer = Producer<'static, Point, DAC_BUFFER_LEN>;
pub type DacConsumer = Consumer<'static, Point, DAC_BUFFER_LEN>;

pub type AdcProducer = Producer<'static, [Point; ADC_COUNT], ADC_BUFFER_LEN>;
pub type AdcConsumer = Consumer<'static, [Point; ADC_COUNT], ADC_BUFFER_LEN>;

#[cfg(feature = "real")]
pub unsafe fn split<T, const N: usize>(rb: &Rb<T, N>) -> (Producer<'_, T, N>, Consumer<'_, T, N>) {
    (Producer::new(rb), Consumer::new(rb))
}
#[cfg(feature = "fake")]
pub unsafe fn split<T, const N: usize>(rb: &Rb<T, N>) -> (Producer<'_, T, N>, Consumer<'_, T, N>) {
    (Producer::new(CachedProd::new(rb)), Consumer::new(CachedCons::new(rb)))
}

lazy_static! {
    pub static ref DAC_BUFFER: Rb<Point, DAC_BUFFER_LEN> = Rb::default();
    pub static ref ADC_BUFFER: Rb<[Point; ADC_COUNT], ADC_BUFFER_LEN> = Rb::default();
}
