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
pub const AO_BUFFER_LEN: usize = 1024;
#[cfg(feature = "real")]
pub const AI_BUFFER_LEN: usize = 384;
#[cfg(feature = "fake")]
pub const AO_BUFFER_LEN: usize = 16384;
#[cfg(feature = "fake")]
pub const AI_BUFFER_LEN: usize = 16384;

#[cfg(feature = "fake")]
pub const BUFFER_TIMEOUT: Option<Duration> = Some(Duration::from_millis(1000));

pub type AoBuffer = Rb<Point, AO_BUFFER_LEN>;
pub type AiBuffer = Rb<[Point; AI_COUNT], AI_BUFFER_LEN>;

pub type AoObserver = Obs<&'static AoBuffer>;
pub type AoProducer = Prod<'static, Point, AO_BUFFER_LEN>;
pub type AoConsumer = Cons<'static, Point, AO_BUFFER_LEN>;

pub type AiProducer = Prod<'static, [Point; AI_COUNT], AI_BUFFER_LEN>;
pub type AiConsumer = Cons<'static, [Point; AI_COUNT], AI_BUFFER_LEN>;

once_mut! {
    pub static mut AO_BUFFER: Rb<Point, AO_BUFFER_LEN> = Rb::default();
    pub static mut AI_BUFFER: Rb<[Point; AI_COUNT], AI_BUFFER_LEN> = Rb::default();
}
