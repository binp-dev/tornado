use common::{config::AI_COUNT, values::Point};
#[cfg(feature = "fake")]
use core::time::Duration;
use once_mut::once_mut;
use ringbuf::{traits::SplitRef, Obs};

#[cfg(feature = "real")]
pub type Rb<T, const N: usize> = ringbuf::StaticRb<T, N>;
#[cfg(feature = "fake")]
pub type Rb<T, const N: usize> = ringbuf_blocking::BlockingStaticRb<T, N>;

pub type Prod<'a, T, const N: usize> = <Rb<T, N> as SplitRef>::RefProd<'a>;
pub type Cons<'a, T, const N: usize> = <Rb<T, N> as SplitRef>::RefCons<'a>;

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
pub type AdcBuffer = Rb<[Point; AI_COUNT], ADC_BUFFER_LEN>;

pub type DacObserver = Obs<&'static DacBuffer>;
pub type DacProducer = Prod<'static, Point, DAC_BUFFER_LEN>;
pub type DacConsumer = Cons<'static, Point, DAC_BUFFER_LEN>;

pub type AdcProducer = Prod<'static, [Point; AI_COUNT], ADC_BUFFER_LEN>;
pub type AdcConsumer = Cons<'static, [Point; AI_COUNT], ADC_BUFFER_LEN>;

once_mut! {
    pub static mut DAC_BUFFER: Rb<Point, DAC_BUFFER_LEN> = Rb::default();
    pub static mut ADC_BUFFER: Rb<[Point; AI_COUNT], ADC_BUFFER_LEN> = Rb::default();
}
