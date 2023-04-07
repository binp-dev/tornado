use common::{config::ADC_COUNT, values::Point};
#[cfg(feature = "fake")]
use core::time::Duration;
use lazy_static::lazy_static;
use ringbuf::StaticRb;
#[cfg(feature = "real")]
use ringbuf::{StaticConsumer, StaticProducer};
#[cfg(feature = "fake")]
use ringbuf_blocking::{Consumer as BlockingConsumer, Producer as BlockingProducer, Rb as BlockingRb};

#[cfg(feature = "real")]
pub type Rb<T, const N: usize> = StaticRb<T, N>;
#[cfg(feature = "real")]
pub type Producer<'a, T, const N: usize> = StaticProducer<'a, T, N>;
#[cfg(feature = "real")]
pub type Consumer<'a, T, const N: usize> = StaticConsumer<'a, T, N>;

#[cfg(feature = "fake")]
pub type Rb<T, const N: usize> = BlockingRb<T, StaticRb<T, N>>;
#[cfg(feature = "fake")]
pub type Producer<'a, T, const N: usize> = BlockingProducer<T, &'a Rb<T, N>>;
#[cfg(feature = "fake")]
pub type Consumer<'a, T, const N: usize> = BlockingConsumer<T, &'a Rb<T, N>>;

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

lazy_static! {
    pub static ref DAC_BUFFER: Rb<Point, DAC_BUFFER_LEN> = Rb::default();
    pub static ref ADC_BUFFER: Rb<[Point; ADC_COUNT], ADC_BUFFER_LEN> = Rb::default();
}
