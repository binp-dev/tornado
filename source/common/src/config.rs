use core::{sync::atomic::AtomicI32, time::Duration};
use flatty::portable::le;

pub const DAC_COUNT: usize = 1;
pub const ADC_COUNT: usize = 6;

pub const SAMPLE_PERIOD: Duration = Duration::from_micros(100);

pub type Point = i32;
pub type PointPortable = le::I32;
pub type AtomicPoint = AtomicI32;

#[cfg(feature = "app")]
pub const DAC_MAX_ABS: f64 = 10.0;
#[cfg(feature = "app")]
pub const ADC_MAX_ABS: f64 = 10.0;

pub const DAC_RAW_OFFSET: Point = 32767;
#[cfg(feature = "app")]
pub const DAC_STEP: f64 = 315.7445 * 1e-6;
#[cfg(feature = "app")]
pub const ADC_STEP: f64 = (346.8012 / 256.0) * 1e-6;

#[cfg(feature = "app")]
pub fn dac_to_volt(point: Point) -> f64 {
    (point - DAC_RAW_OFFSET) as f64 * DAC_STEP
}
#[cfg(feature = "app")]
pub fn volt_to_dac(volt: f64) -> Point {
    (volt / DAC_STEP) as Point + DAC_RAW_OFFSET
}
#[cfg(feature = "app")]
pub fn adc_to_volt(point: Point) -> f64 {
    point as f64 * ADC_STEP
}
#[cfg(feature = "app")]
pub fn volt_to_adc(volt: f64) -> Point {
    (volt / ADC_STEP) as Point
}

pub const MAX_APP_MSG_LEN: usize = 496;
pub const MAX_MCU_MSG_LEN: usize = 496;

pub const KEEP_ALIVE_PERIOD: Duration = Duration::from_millis(100);
pub const KEEP_ALIVE_MAX_DELAY: Duration = Duration::from_millis(200);

#[cfg(feature = "fake")]
pub const CHANNEL_ADDR: &str = "localhost:4578";
