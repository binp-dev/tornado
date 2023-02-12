use core::time::Duration;
use flatty::portable::le;

pub const DAC_COUNT: usize = 1;
pub const ADC_COUNT: usize = 6;

pub const SAMPLE_FREQ_HZ: usize = 10000;

pub type Point = i32;
pub type PointPortable = le::I32;

#[cfg(feature = "app")]
pub const DAC_MAX_ABS_V: f64 = 10.0;
#[cfg(feature = "app")]
pub const ADC_MAX_ABS_V: f64 = 10.0;

pub const DAC_CODE_SHIFT: Point = 32767;
#[cfg(feature = "app")]
pub const DAC_STEP_UV: f64 = 315.7445;
#[cfg(feature = "app")]
pub const ADC_STEP_UV: f64 = 346.8012;

pub const MAX_APP_MSG_LEN: usize = 496;
pub const MAX_MCU_MSG_LEN: usize = 496;

pub const KEEP_ALIVE_PERIOD: Duration = Duration::from_millis(100);
pub const KEEP_ALIVE_MAX_DELAY: Duration = Duration::from_millis(200);
