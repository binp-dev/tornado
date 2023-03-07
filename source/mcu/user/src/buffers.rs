use common::{
    config::ADC_COUNT,
    units::{AdcPoint, DacPoint},
};
use lazy_static::lazy_static;
use ringbuf::{StaticConsumer, StaticProducer, StaticRb};

#[cfg(feature = "real")]
pub const DAC_BUFFER_LEN: usize = 1024;
#[cfg(feature = "real")]
pub const ADC_BUFFER_LEN: usize = 384;
#[cfg(feature = "fake")]
pub const DAC_BUFFER_LEN: usize = 16 * 1024;
#[cfg(feature = "fake")]
pub const ADC_BUFFER_LEN: usize = 16 * 384;

pub type DacBuffer = StaticRb<DacPoint, DAC_BUFFER_LEN>;
pub type AdcBuffer = StaticRb<[AdcPoint; ADC_COUNT], ADC_BUFFER_LEN>;

pub type DacProducer = StaticProducer<'static, DacPoint, DAC_BUFFER_LEN>;
pub type DacConsumer = StaticConsumer<'static, DacPoint, DAC_BUFFER_LEN>;

pub type AdcProducer = StaticProducer<'static, [AdcPoint; ADC_COUNT], ADC_BUFFER_LEN>;
pub type AdcConsumer = StaticConsumer<'static, [AdcPoint; ADC_COUNT], ADC_BUFFER_LEN>;

lazy_static! {
    pub static ref DAC_BUFFER: StaticRb<DacPoint, DAC_BUFFER_LEN> = StaticRb::default();
    pub static ref ADC_BUFFER: StaticRb<[AdcPoint; ADC_COUNT], ADC_BUFFER_LEN> = StaticRb::default();
}
