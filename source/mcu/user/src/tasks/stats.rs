use crate::println;
use alloc::sync::Arc;
use common::{
    config::AI_COUNT,
    values::{AtomicUv, Uv},
};
use core::{
    fmt::{self, Display, Formatter, Write},
    sync::atomic::{AtomicI8, AtomicU8, AtomicUsize, Ordering},
    time::Duration,
};
use indenter::indented;
use lazy_static::lazy_static;
use ustd::task::{self, BlockingContext};

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
    sync_count: AtomicUsize,
    /// Number of SkifIO `SMP_RDY` signals captured.
    ready_count: AtomicUsize,
    /// Number of AI/AO samples.
    sample_count: AtomicUsize,
    intrs_per_sample: AtomicUsize,
    /// Maximum number of `SMP_RDY` per SkifIO communication session.
    /// If it isn't equal to `1` that means that we lose some signals.
    max_intrs_per_sample: AtomicUsize,
    /// Count of CRC16 mismatches in SkifIO communication.
    crc_error_count: AtomicUsize,
    /// Count of IOC being disconnected
    ioc_drop_count: AtomicUsize,
    /// SkifIO controller temperature.
    skifio_temp: AtomicI8,
    /// SkifIO board status.
    skifio_status: AtomicU8,

    pub ao: StatsAo,
    pub ais: StatsAis,
}

#[derive(Default)]
pub struct StatsAo {
    /// Number of points lost because the AO buffer was empty.
    lost_empty: AtomicUsize,
    /// Number of points lost because the AO buffer was full.
    lost_full: AtomicUsize,
    /// IOC sent more points than were requested.
    req_exceed: AtomicUsize,

    value: ValueStats,
}

#[derive(Default)]
pub struct StatsAis {
    /// Number of points lost because the AI buffer was full.
    lost_full: AtomicUsize,

    values: [ValueStats; AI_COUNT],
}

#[derive(Default)]
pub struct ValueStats {
    count: AtomicUsize,
    last: AtomicUv,
    min: AtomicUv,
    max: AtomicUv,
}

impl Statistics {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        self.sync_count.store(0, Ordering::Relaxed);
        self.ready_count.store(0, Ordering::Relaxed);
        self.sample_count.store(0, Ordering::Relaxed);
        self.max_intrs_per_sample.store(0, Ordering::Relaxed);
        self.crc_error_count.store(0, Ordering::Relaxed);
        self.ioc_drop_count.store(0, Ordering::Relaxed);
        self.skifio_temp.store(i8::MIN, Ordering::Relaxed);
        self.skifio_status.store(0, Ordering::Relaxed);

        self.ao.reset();
        self.ais.reset();
    }

    fn intr_clock(&self) {
        self.sync_count.fetch_add(1, Ordering::Relaxed);
    }
    fn intr_sample(&self) {
        self.intrs_per_sample.fetch_add(1, Ordering::Relaxed);
        self.ready_count.fetch_add(1, Ordering::Relaxed);
    }

    pub fn report_sample(&self) {
        let intrs = self.intrs_per_sample.swap(0, Ordering::Relaxed);
        self.max_intrs_per_sample.fetch_max(intrs, Ordering::Relaxed);

        self.sample_count.fetch_add(1, Ordering::Relaxed);
    }
    pub fn report_crc_error(&self) {
        self.crc_error_count.fetch_add(1, Ordering::Relaxed);
    }
    pub fn report_ioc_drop(&self) {
        self.ioc_drop_count.fetch_add(1, Ordering::Relaxed);
    }
    pub fn set_skifio_temp(&self, temp: i8) {
        self.skifio_temp.store(temp, Ordering::Relaxed);
    }
    pub fn set_skifio_status(&self, status: u8) {
        self.skifio_status.store(status, Ordering::Relaxed);
    }
}

impl StatsAo {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        self.lost_empty.store(0, Ordering::Relaxed);
        self.lost_full.store(0, Ordering::Relaxed);
        self.req_exceed.store(0, Ordering::Relaxed);

        self.value.reset();
    }

    pub fn report_lost_empty(&self, count: usize) {
        self.lost_empty.fetch_add(count, Ordering::Relaxed);
        #[cfg(feature = "fake")]
        panic!("AO ring buffer is empty");
    }
    pub fn report_lost_full(&self, count: usize) {
        self.lost_full.fetch_add(count, Ordering::Relaxed);
        #[cfg(feature = "fake")]
        panic!("AO ring buffer is full");
    }
    pub fn report_req_exceed(&self, count: usize) {
        self.req_exceed.fetch_add(count, Ordering::Relaxed);
        #[cfg(feature = "fake")]
        panic!("IOC sent more points than have been requested");
    }
    pub fn update_value(&self, value: Uv) {
        self.value.update(value);
    }
}

impl StatsAis {
    pub fn new() -> Self {
        let this = Self::default();
        this.reset();
        this
    }
    pub fn reset(&self) {
        self.lost_full.store(0, Ordering::Relaxed);
        self.values.iter().for_each(ValueStats::reset);
    }

    pub fn report_lost_full(&self, count: usize) {
        self.lost_full.fetch_add(count, Ordering::Relaxed);
        #[cfg(feature = "fake")]
        panic!("AI ring buffer is full");
    }
    pub fn update_values(&self, values: [Uv; AI_COUNT]) {
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
        self.count.store(0, Ordering::Relaxed);
        self.max.store(Uv::MIN, Ordering::Relaxed);
        self.min.store(Uv::MAX, Ordering::Relaxed);
        self.last.store(Uv::default(), Ordering::Relaxed);
    }
    pub fn update(&self, value: Uv) {
        self.min.fetch_min(value, Ordering::Relaxed);
        self.max.fetch_max(value, Ordering::Relaxed);
        self.last.store(value, Ordering::Relaxed);
        self.count.fetch_add(1, Ordering::Relaxed);
    }
}

impl Display for Statistics {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
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
        writeln!(f, "ioc_drop_count: {}", self.ioc_drop_count.load(Ordering::Relaxed))?;
        writeln!(f, "skifio_temp: {}", self.skifio_temp.load(Ordering::Relaxed))?;
        writeln!(f, "skifio_status: 0b{:08b}", self.skifio_status.load(Ordering::Relaxed))?;

        writeln!(f, "ao:")?;
        write!(indented(f), "{}", self.ao)?;

        writeln!(f, "ais:")?;
        write!(indented(f), "{}", self.ais)?;

        Ok(())
    }
}

impl Display for StatsAo {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        writeln!(f, "lost_empty: {}", self.lost_empty.load(Ordering::Relaxed))?;
        writeln!(f, "lost_full: {}", self.lost_full.load(Ordering::Relaxed))?;
        writeln!(f, "req_exceed: {}", self.req_exceed.load(Ordering::Relaxed))?;

        writeln!(f, "value:")?;
        write!(indented(f), "{}", self.value)?;

        Ok(())
    }
}

impl Display for StatsAis {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        writeln!(f, "lost_full: {}", self.lost_full.load(Ordering::Relaxed))?;

        for (i, ai) in self.values.iter().enumerate() {
            writeln!(f, "{}:", i)?;
            write!(indented(f), "{}", ai)?;
        }
        Ok(())
    }
}

impl Display for ValueStats {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        let count = self.count.load(Ordering::Relaxed);
        writeln!(f, "count: {}", count)?;
        if count != 0 {
            writeln!(f, "last: {}", &self.last.load(Ordering::Relaxed))?;
            writeln!(f, "min: {}", &self.min.load(Ordering::Relaxed))?;
            writeln!(f, "max: {}", &self.max.load(Ordering::Relaxed))?;
        }

        Ok(())
    }
}

impl Statistics {
    pub fn run_printer(self: Arc<Self>, period: Duration) {
        task::Builder::new()
            .name("stats")
            .priority(1)
            .spawn(move |cx| loop {
                cx.sleep(Some(period));
                println!();
                println!("[Statistics]");
                println!("{}", self);
            })
            .unwrap();
    }
}
