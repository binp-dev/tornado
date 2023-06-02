use common::{config::ADC_COUNT, values::Point};
#[cfg(feature = "fake")]
use core::time::Duration;
use once_mut::once_mut;
use ringbuf::{CachedCons, CachedProd, Obs, StaticRb};
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

pub type DacObserver = Obs<&'static DacBuffer>;
pub type DacProducer = Producer<'static, Point, DAC_BUFFER_LEN>;
pub type DacConsumer = Consumer<'static, Point, DAC_BUFFER_LEN>;

pub type AdcProducer = Producer<'static, [Point; ADC_COUNT], ADC_BUFFER_LEN>;
pub type AdcConsumer = Consumer<'static, [Point; ADC_COUNT], ADC_BUFFER_LEN>;

once_mut! {
    pub static mut DAC_BUFFER: Rb<Point, DAC_BUFFER_LEN> = Rb::default();
    pub static mut ADC_BUFFER: Rb<[Point; ADC_COUNT], ADC_BUFFER_LEN> = Rb::default();
}
