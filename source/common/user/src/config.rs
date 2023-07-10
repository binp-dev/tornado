use core::time::Duration;

pub const AI_COUNT: usize = 6;

pub const DIN_BITS: usize = 8;
pub const DOUT_BITS: usize = 4;

pub const SAMPLE_PERIOD: Duration = Duration::from_micros(100);

pub const MAX_APP_MSG_LEN: usize = 496;
pub const MAX_MCU_MSG_LEN: usize = 496;

pub const KEEP_ALIVE_PERIOD: Duration = Duration::from_millis(100);
pub const KEEP_ALIVE_MAX_DELAY: Duration = Duration::from_millis(200);

#[cfg(feature = "fake")]
pub const CHANNEL_HOST: &str = "localhost";
#[cfg(feature = "fake")]
pub const CHANNEL_PORT: u16 = 4578;
