use super::control::AdcPoints;
use core::sync::atomic::{AtomicU64, AtomicUsize};

pub struct StatsDac {
    pub lost_empty: AtomicUsize,
    pub lost_full: AtomicUsize,
    pub req_exceed: AtomicUsize,
}
pub struct StatsAdc {
    pub lost_full: AtomicUsize,
}
pub struct Statistics {
    pub sample_count: AtomicU64,
    pub dac: StatsDac,
    pub adcs: StatsAdc,
    pub crc_error_count: AtomicU64,
}

impl StatsAdc {
    pub fn update_values(&self, _values: AdcPoints) {
        unimplemented!()
    }
}
impl Statistics {
    pub fn reset(&self) {
        unimplemented!()
    }
}
