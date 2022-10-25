include!(concat!(env!("OUT_DIR"), "/config.rs"));

use flatty::portable::le;
use std::time::Duration;

pub type PointPortable = le::I32;

pub const KEEP_ALIVE_PERIOD: Duration = Duration::from_millis(100);
