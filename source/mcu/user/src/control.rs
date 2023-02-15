use crate::{
    hal::RetCode,
    println,
    skifio::{self, Aout, AtomicDin, AtomicDout, Din, DinHandler, XferIn, XferOut},
    Error,
};
use alloc::sync::Arc;
use common::config::{Point, ADC_COUNT};
use core::{
    sync::atomic::{AtomicBool, AtomicU64, Ordering},
    time::Duration,
};
use freertos::{Semaphore, Task, TaskPriority};
use ringbuf::{StaticConsumer, StaticProducer};

pub type DacPoint = Point;
pub type AdcPoint = [Point; ADC_COUNT];

pub const DAC_BUFFER_LEN: usize = 1024;
pub const ADC_BUFFER_LEN: usize = 384;

pub type DacProducer = StaticProducer<'static, DacPoint, DAC_BUFFER_LEN>;
pub type DacConsumer = StaticConsumer<'static, DacPoint, DAC_BUFFER_LEN>;

pub type AdcProducer = StaticProducer<'static, AdcPoint, ADC_BUFFER_LEN>;
pub type AdcConsumer = StaticConsumer<'static, AdcPoint, ADC_BUFFER_LEN>;

pub struct StatsDac {
    pub lost_empty: AtomicU64,
}
pub struct StatsAdc {
    pub lost_full: AtomicU64,
}
pub struct Statistics {
    pub sample_count: AtomicU64,
    pub dac: StatsDac,
    pub adc: StatsAdc,
    pub crc_error_count: AtomicU64,
}

impl StatsAdc {
    pub fn update_values(&self, _values: AdcPoint) {
        unimplemented!()
    }
}

/// Number of DAC points to write until notified.
// FIXME: Adjust value.
const DAC_NOTIFY_EVERY: usize = 100;
/// Number of ADC points to read until notified.
// FIXME: Adjust value.
const ADC_NOTIFY_EVERY: usize = 100;

pub struct ControlHandle {
    /// Semaphore to notify that something is ready.
    ready_sem: Semaphore,

    dac_enabled: AtomicBool,

    din: AtomicDin,
    dout: AtomicDout,

    /// Discrete input has changed.
    din_changed: AtomicBool,
    /// Discrete output has changed.
    dout_changed: AtomicBool,
}

struct ControlDac {
    running: bool,
    buffer: DacConsumer,
    last_point: DacPoint,
    counter: usize,
}

struct ControlAdc {
    buffer: AdcProducer,
    last_point: AdcPoint,
    counter: usize,
}

pub struct Control {
    dac: ControlDac,
    adc: ControlAdc,
    handle: Arc<ControlHandle>,
}

impl ControlHandle {
    fn new() -> Self {
        Self {
            ready_sem: Semaphore::new_binary().unwrap(),
            dac_enabled: AtomicBool::new(false),
            din: AtomicDin::new(0),
            dout: AtomicDout::new(0),
            din_changed: AtomicBool::new(false),
            dout_changed: AtomicBool::new(false),
        }
    }

    fn update_din(&self, din: Din) -> bool {
        if self.din.swap(din, Ordering::AcqRel) != din {
            self.din_changed.fetch_or(true, Ordering::AcqRel);
            true
        } else {
            false
        }
    }
}

impl Control {
    pub fn new(dac_buf: DacConsumer, adc_buf: AdcProducer) -> (Self, Arc<ControlHandle>) {
        let handle = Arc::new(ControlHandle::new());
        (
            Self {
                dac: ControlDac {
                    running: false,
                    buffer: dac_buf,
                    last_point: 0x7fff,
                    counter: 0,
                },
                adc: ControlAdc {
                    buffer: adc_buf,
                    last_point: AdcPoint::default(),
                    counter: 0,
                },
                handle: handle.clone(),
            },
            handle,
        )
    }

    fn make_din_handler(&self) -> impl DinHandler {
        let handle = self.handle.clone();
        move |context, din| {
            if handle.update_din(din) {
                handle.ready_sem.give_from_isr(context);
            }
        }
    }

    fn task_main(&mut self, stats: Arc<Statistics>) {
        let mut skifio = skifio::handle().unwrap();
        skifio.subscribe_din(Some(self.make_din_handler())).unwrap();

        println!("Enter SkifIO loop");
        //uint64_t prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
        let iter_counter = 0;
        loop {
            let mut ready = false;

            skifio
                .set_dac_state(self.handle.dac_enabled.load(Ordering::Acquire))
                .unwrap();

            // Wait for 10 kHz sync signal
            match skifio.wait_ready(Some(Duration::from_millis(1000))) {
                Ok(()) => (),
                Err(Error::Hal(RetCode::TimedOut)) => {
                    println!("SkifIO timeout {}", iter_counter);
                    continue;
                }
                Err(e) => panic!("{:?}", e),
            }

            // Write discrete output
            if self.handle.dout_changed.fetch_and(false, Ordering::AcqRel) {
                skifio
                    .write_dout(self.handle.dout.load(Ordering::Acquire))
                    .unwrap();
            }

            // Read discrete input
            ready |= self.handle.update_din(skifio.read_din());

            // Statistics: detect 10 kHz sync signal loss
            /*
            self->stats->max_intrs_per_sample = hal_max(
                self->stats->max_intrs_per_sample,
                (uint32_t)(_SKIFIO_DEBUG_INFO.intr_count - prev_intr_count) //
            );
            prev_intr_count = _SKIFIO_DEBUG_INFO.intr_count;
            */

            // Fetch next DAC value from buffer
            let mut dac_value = self.dac.last_point;
            if self.dac.running {
                if let Some(value) = self.dac.buffer.pop() {
                    dac_value = value;
                    self.dac.last_point = value;
                    // Decrement DAC notification counter.
                    if self.dac.counter > 0 {
                        self.dac.counter -= 1;
                    } else {
                        self.dac.counter = DAC_NOTIFY_EVERY - 1;
                        ready = true;
                    }
                } else {
                    stats.dac.lost_empty.fetch_add(1, Ordering::AcqRel);
                }
            }

            // Transfer DAC/ADC values to/from SkifIO board.
            {
                // TODO: Check for overflow.
                let dac = dac_value as Aout;
                let adcs = match skifio.transfer(XferOut { dac }) {
                    Ok(XferIn { adcs }) => {
                        self.adc.last_point = adcs;
                        adcs
                    }
                    Err(Error::Hal(RetCode::InvalidData)) => {
                        // CRC check error
                        stats.crc_error_count.fetch_add(1, Ordering::AcqRel);
                        self.adc.last_point
                    }
                    Err(e) => panic!("{:?}", e),
                };

                // Handle ADCs
                {
                    // Update ADC value statistics
                    stats.adc.update_values(adcs);
                    // Push ADC point to buffer.
                    if self.adc.buffer.push(adcs).is_err() {
                        stats.adc.lost_full.fetch_add(1, Ordering::AcqRel);
                    }

                    // Decrement ADC notification counter.
                    if self.adc.counter > 0 {
                        self.adc.counter -= 1;
                    } else {
                        self.adc.counter = ADC_NOTIFY_EVERY - 1;
                        ready = true;
                    }
                }
            }

            if ready {
                // Notify
                self.handle.ready_sem.give();
            }

            stats.sample_count.fetch_add(1, Ordering::AcqRel);
        }
    }

    pub fn run(mut self, priority: u8, stats: Arc<Statistics>) -> Result<Task, Error> {
        Task::new()
            .name("control")
            .priority(TaskPriority(priority))
            .start(move |_| self.task_main(stats))
            .map_err(|e| e.into())
    }
}
