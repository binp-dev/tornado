use std::time::Duration;

pub const DAC_COUNT: usize = 1;
pub const ADC_COUNT: usize = 6;

pub type Point = i32;

pub const MAX_APP_MSG_LEN: usize = 496;
pub const MAX_MCU_MSG_LEN: usize = 496;

pub const KEEP_ALIVE_PERIOD: Duration = Duration::from_millis(100);
