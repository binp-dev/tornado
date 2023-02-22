use crate::println;

use crate::buffers::AdcPoints;
use alloc::sync::Arc;
use common::config::{AtomicPoint, Point, ADC_COUNT};
use core::{
    fmt::{self, Display, Formatter, Write},
    sync::atomic::{fence, AtomicUsize, Ordering},
    time::Duration,
};
use indenter::indented;
use lazy_static::lazy_static;
use portable_atomic::{AtomicI64, AtomicU64};
use ustd::task;

lazy_static! {
    pub static ref STATISTICS: Arc<Statistics> = Arc::new(Statistics::new());
}

#[no_mangle]
extern "C" fn user_sync_intr() {
    STATISTICS.intr_clock();
}
#[no_mangle]
extern "C" fn user_sample_intr() {
    STATISTICS.intr_sample();
}

#[derive(Default)]
pub struct Statistics {
    /// Number of 10 kHz sync signals captured.
    sync_count: AtomicU64,
    /// Number of SkifIO `SMP_RDY` signals captured.
    ready_count: AtomicU64,
    /// Number of ADC/DAC samples.
    sample_count: AtomicU64,
    intrs_per_sample: AtomicUsize,
    /// Maximum number of `SMP_RDY` per SkifIO communication session.
    /// If it isn't equal to `1` that means that we lose some signals.
    max_intrs_per_sample: AtomicUsize,
    /// Count of CRC16 mismatches in SkifIO communication.
    crc_error_count: AtomicU64,

    pub dac: StatsDac,
    pub adcs: StatsAdc,
}

#[derive(Default)]
pub struct StatsDac {
    /// Number of points lost because the DAC buffer was empty.
    lost_empty: AtomicU64,
    /// Number of points lost because the DAC buffer was full.
    lost_full: AtomicU64,
    /// IOC sent more points than were requested.
    req_exceed: AtomicU64,

    value: ValueStats,
}

#[derive(Default)]
pub struct StatsAdc {
    /// Number of points lost because the ADC buffer was full.
    lost_full: AtomicU64,
    values: [ValueStats; ADC_COUNT],
}

#[derive(Default)]
struct ValueStats {
    sum: AtomicI64,
    count: AtomicU64,
    last: AtomicPoint,
    min: AtomicPoint,
    max: AtomicPoint,
}

impl Statistics {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        fence(Ordering::Acquire);
        self.sync_count.store(0, Ordering::Relaxed);
        self.ready_count.store(0, Ordering::Relaxed);
        self.sample_count.store(0, Ordering::Relaxed);
        self.max_intrs_per_sample.store(0, Ordering::Relaxed);
        self.crc_error_count.store(0, Ordering::Relaxed);
        fence(Ordering::Release);

        self.dac.reset();
        self.adcs.reset();
    }

    fn intr_clock(&self) {
        self.sync_count.fetch_add(1, Ordering::AcqRel);
    }
    fn intr_sample(&self) {
        self.intrs_per_sample.fetch_add(1, Ordering::AcqRel);
        self.ready_count.fetch_add(1, Ordering::AcqRel);
    }

    pub fn report_sample(&self) {
        let intrs = self.intrs_per_sample.swap(0, Ordering::AcqRel);
        self.max_intrs_per_sample.fetch_max(intrs, Ordering::AcqRel);

        self.sample_count.fetch_add(1, Ordering::AcqRel);
    }
    pub fn report_crc_error(&self) {
        self.crc_error_count.fetch_add(1, Ordering::AcqRel);
    }
}

impl StatsDac {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        fence(Ordering::Acquire);
        self.lost_empty.store(0, Ordering::Relaxed);
        self.lost_full.store(0, Ordering::Relaxed);
        self.lost_full.store(0, Ordering::Relaxed);
        fence(Ordering::Release);

        self.value.reset();
    }

    pub fn report_lost_empty(&self, count: usize) {
        self.lost_empty.fetch_add(count as u64, Ordering::AcqRel);
    }
    pub fn report_lost_full(&self, count: usize) {
        self.lost_full.fetch_add(count as u64, Ordering::AcqRel);
    }
    pub fn report_req_exceed(&self, count: usize) {
        self.req_exceed.fetch_add(count as u64, Ordering::AcqRel);
    }
    pub fn update_value(&self, value: Point) {
        self.value.update(value);
    }
}

impl StatsAdc {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        self.lost_full.store(0, Ordering::Release);
        self.values.iter().for_each(ValueStats::reset);
    }

    pub fn report_lost_full(&self, count: usize) {
        self.lost_full.fetch_add(count as u64, Ordering::AcqRel);
    }
    pub fn update_values(&self, values: AdcPoints) {
        self.values.iter().zip(values).for_each(|(v, x)| v.update(x));
    }
}

impl ValueStats {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        fence(Ordering::Acquire);
        self.count.store(0, Ordering::Relaxed);
        self.sum.store(0, Ordering::Relaxed);
        self.max.store(Point::MIN, Ordering::Relaxed);
        self.min.store(Point::MAX, Ordering::Relaxed);
        self.last.store(0, Ordering::Relaxed);
        fence(Ordering::Release);
    }
    pub fn update(&self, value: Point) {
        fence(Ordering::Acquire);
        self.min.fetch_min(value, Ordering::Relaxed);
        self.max.fetch_max(value, Ordering::Relaxed);
        self.last.store(value, Ordering::Relaxed);
        self.sum.fetch_add(value as i64, Ordering::Relaxed);
        self.count.fetch_add(1, Ordering::Relaxed);
        fence(Ordering::Release);
    }
}

impl Display for Statistics {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        fence(Ordering::Acquire);
        let sync = self.sync_count.load(Ordering::Relaxed);
        let ready = self.ready_count.load(Ordering::Relaxed);
        let sample = self.sample_count.load(Ordering::Relaxed);

        writeln!(f, "sync_count: {}", sync)?;
        writeln!(f, "ready_count: {}", ready)?;
        writeln!(f, "sample_count: {}", sample)?;
        writeln!(
            f,
            "max_intrs_per_sample: {}",
            self.max_intrs_per_sample.load(Ordering::Relaxed)
        )?;
        writeln!(f, "crc_error_count: {}", self.crc_error_count.load(Ordering::Relaxed))?;

        writeln!(f, "dac:")?;
        write!(indented(f), "{}", self.dac)?;

        writeln!(f, "adcs:")?;
        write!(indented(f), "{}", self.adcs)?;

        Ok(())
    }
}

impl Display for StatsDac {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        fence(Ordering::Acquire);
        writeln!(f, "lost_empty: {}", self.lost_empty.load(Ordering::Relaxed))?;
        writeln!(f, "lost_full: {}", self.lost_full.load(Ordering::Relaxed))?;
        writeln!(f, "req_exceed: {}", self.req_exceed.load(Ordering::Relaxed))?;

        writeln!(f, "value:")?;
        write!(indented(f), "{}", self.value)?;

        Ok(())
    }
}

impl Display for StatsAdc {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        writeln!(f, "lost_full: {}", self.lost_full.load(Ordering::Acquire))?;
        for (i, adc) in self.values.iter().enumerate() {
            writeln!(f, "{}:", i)?;
            write!(indented(f), "{}", adc)?;
        }
        Ok(())
    }
}

macro_rules! format_value {
    ($value:expr) => {
        format_args!("0x{value:08x} == {value}", value = $value)
    };
}

impl Display for ValueStats {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        fence(Ordering::Acquire);

        let count = self.count.load(Ordering::Relaxed);
        writeln!(f, "count: {}", count)?;
        if count != 0 {
            writeln!(f, "last: {}", format_value!(&self.last.load(Ordering::Relaxed)))?;

            writeln!(f, "min: {}", format_value!(&self.min.load(Ordering::Relaxed)))?;
            writeln!(f, "max: {}", format_value!(&self.max.load(Ordering::Relaxed)))?;

            let avg = (self.sum.load(Ordering::Relaxed) / count as i64) as Point;
            writeln!(f, "avg: {}", format_value!(&avg))?;
        }

        Ok(())
    }
}

impl Statistics {
    pub fn run_printer(self: Arc<Self>, period: Duration) {
        task::spawn(task::Priority(1), move || loop {
            task::sleep(period);
            println!();
            println!("[Statistics]");
            println!("{}", self);
        })
        .unwrap();
    }
}
