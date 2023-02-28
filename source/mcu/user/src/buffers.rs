use common::config::{Point, ADC_COUNT};
use lazy_static::lazy_static;
use ringbuf::{StaticConsumer, StaticProducer, StaticRb};

pub type AdcPoints = [Point; ADC_COUNT];

#[cfg(feature = "real")]
pub const DAC_BUFFER_LEN: usize = 1024;
#[cfg(feature = "real")]
pub const ADC_BUFFER_LEN: usize = 384;
#[cfg(feature = "emul")]
pub const DAC_BUFFER_LEN: usize = 16 * 1024;
#[cfg(feature = "emul")]
pub const ADC_BUFFER_LEN: usize = 16 * 384;

pub type DacBuffer = StaticRb<Point, DAC_BUFFER_LEN>;
pub type AdcBuffer = StaticRb<AdcPoints, ADC_BUFFER_LEN>;

pub type DacProducer = StaticProducer<'static, Point, DAC_BUFFER_LEN>;
pub type DacConsumer = StaticConsumer<'static, Point, DAC_BUFFER_LEN>;

pub type AdcProducer = StaticProducer<'static, AdcPoints, ADC_BUFFER_LEN>;
pub type AdcConsumer = StaticConsumer<'static, AdcPoints, ADC_BUFFER_LEN>;

lazy_static! {
    pub static ref DAC_BUFFER: StaticRb<Point, DAC_BUFFER_LEN> = StaticRb::default();
    pub static ref ADC_BUFFER: StaticRb<AdcPoints, ADC_BUFFER_LEN> = StaticRb::default();
}
