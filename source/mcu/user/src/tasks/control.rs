use super::stats::Statistics;
#[cfg(feature = "fake")]
use crate::buffers::BUFFER_TIMEOUT;
#[cfg(feature = "real")]
use crate::skifio::SkifioIface as _;
use crate::{
    buffers::{AdcProducer, DacConsumer},
    error::{Error, ErrorKind},
    println,
    skifio::{self, DinHandler, XferIn, XferOut},
};
use alloc::{boxed::Box, sync::Arc};
use common::{
    config::ADC_COUNT,
    values::{AtomicBits, AtomicUv, Din, Dout, Point, PointOpt, Uv},
};
use core::{
    sync::atomic::{AtomicBool, AtomicUsize, Ordering},
    time::Duration,
};
use ustd::{
    sync::Semaphore,
    task::{self, BlockingContext, Context, Priority, TaskContext},
};

pub struct ControlHandle {
    /// Semaphore to notify that something is ready.
    ready_sem: Semaphore,

    dac_enabled: AtomicBool,
    #[cfg(feature = "fake")]
    dac_enable_sem: Semaphore,

    pub dac_add: AtomicUv,

    din: AtomicBits,
    pub dout: AtomicBits,

    /// Discrete input has changed.
    din_changed: AtomicBool,
    /// Discrete output has changed.
    dout_changed: AtomicBool,

    /// Number of DAC points to write until notified.
    dac_notify_every: AtomicUsize,
    /// Number of ADC points to read until notified.
    adc_notify_every: AtomicUsize,
}

struct ControlDac {
    buffer: DacConsumer,
    last_point: Uv,
    counter: usize,
}

struct ControlAdc {
    buffer: AdcProducer,
    last_point: [Uv; ADC_COUNT],
    counter: usize,
}

pub struct Control {
    dac: ControlDac,
    adc: ControlAdc,
    handle: Arc<ControlHandle>,
    stats: Arc<Statistics>,
}

impl ControlHandle {
    fn new() -> Self {
        Self {
            ready_sem: Semaphore::new().unwrap(),
            dac_enabled: AtomicBool::new(false),
            #[cfg(feature = "fake")]
            dac_enable_sem: Semaphore::new().unwrap(),
            dac_add: AtomicUv::default(),
            din: AtomicBits::default(),
            dout: AtomicBits::default(),
            din_changed: AtomicBool::new(false),
            dout_changed: AtomicBool::new(false),
            dac_notify_every: AtomicUsize::new(0),
            adc_notify_every: AtomicUsize::new(0),
        }
    }
    pub fn configure(&self, dac_notify_every: usize, adc_notify_every: usize) {
        self.dac_notify_every.store(dac_notify_every, Ordering::Release);
        self.adc_notify_every.store(adc_notify_every, Ordering::Release);
    }

    pub fn notify(&self, cx: &mut impl Context) {
        self.ready_sem.try_give(cx);
    }
    pub fn wait_ready(&self, cx: &mut impl BlockingContext, timeout: Option<Duration>) -> bool {
        self.ready_sem.take(cx, timeout)
    }

    pub fn set_dac_mode(&self, _cx: &mut impl Context, enabled: bool) {
        self.dac_enabled.store(enabled, Ordering::Release);
        #[cfg(feature = "fake")]
        if enabled {
            self.dac_enable_sem.try_give(_cx);
        }
    }

    fn update_din(&self, value: Din) -> bool {
        if self.din.swap(value.into(), Ordering::AcqRel) != value.into() {
            self.din_changed.fetch_or(true, Ordering::AcqRel);
            true
        } else {
            false
        }
    }
    pub fn take_din(&self) -> Option<Din> {
        if self.din_changed.fetch_and(false, Ordering::AcqRel) {
            Some(self.din.load(Ordering::Acquire).try_into().unwrap())
        } else {
            None
        }
    }

    pub fn set_dout(&self, value: Dout) {
        if self.dout.swap(value.into(), Ordering::AcqRel) != value.into() {
            self.dout_changed.fetch_or(true, Ordering::AcqRel);
        }
    }
}

impl Control {
    pub fn new(dac_buf: DacConsumer, adc_buf: AdcProducer, stats: Arc<Statistics>) -> (Self, Arc<ControlHandle>) {
        let handle = Arc::new(ControlHandle::new());
        (
            Self {
                dac: ControlDac {
                    buffer: dac_buf,
                    last_point: Uv::default(),
                    counter: 0,
                },
                adc: ControlAdc {
                    buffer: adc_buf,
                    last_point: [Uv::default(); ADC_COUNT],
                    counter: 0,
                },
                handle: handle.clone(),
                stats,
            },
            handle,
        )
    }

    fn make_din_handler(&self) -> Box<dyn DinHandler> {
        let handle = self.handle.clone();
        Box::new(move |cx, din| {
            if handle.update_din(din) {
                handle.ready_sem.try_give(cx);
            }
        })
    }

    fn task_main(mut self, cx: &mut TaskContext) -> ! {
        let handle = self.handle.clone();
        let stats = self.stats.clone();

        let mut skifio = skifio::handle().unwrap();
        skifio.subscribe_din(Some(self.make_din_handler())).unwrap();

        #[cfg(feature = "fake")]
        while !handle.dac_enabled.load(Ordering::Acquire) {
            if !handle.dac_enable_sem.take(cx, BUFFER_TIMEOUT) {
                println!("DAC enable timeout");
            }
        }

        println!("Enter SkifIO loop");
        loop {
            let mut ready = false;

            skifio.set_dac_state(handle.dac_enabled.load(Ordering::Acquire)).unwrap();

            // Wait for 10 kHz sync signal
            match skifio.wait_ready(Some(Duration::from_millis(1000))) {
                Ok(()) => (),
                Err(Error {
                    kind: ErrorKind::TimedOut,
                    ..
                }) => {
                    println!("SkifIO timeout");
                    continue;
                }
                Err(e) => panic!("{:?}", e),
            }

            // Write discrete output
            if handle.dout_changed.fetch_and(false, Ordering::AcqRel) {
                skifio
                    .write_dout(handle.dout.load(Ordering::Acquire).try_into().unwrap())
                    .unwrap();
            }

            // Read discrete input
            ready |= handle.update_din(skifio.read_din());

            // Fetch next DAC value from buffer
            let mut dac = self.dac.last_point;
            if handle.dac_enabled.load(Ordering::Acquire) {
                #[cfg(feature = "fake")]
                while !self.dac.buffer.wait(1, BUFFER_TIMEOUT) {
                    println!("DAC buffer timeout");
                }

                let mut empty = true;
                while let Some(p) = self.dac.buffer.pop() {
                    match p.into_opt() {
                        PointOpt::Uv(value) => {
                            dac = value;
                            self.dac.last_point = value;
                            // Increment DAC notification counter.
                            self.dac.counter += 1;
                            if self.dac.counter >= handle.dac_notify_every.load(Ordering::Acquire) {
                                self.dac.counter = 0;
                                ready = true;
                            }
                            empty = false;
                            break;
                        }
                        // TODO: Handle separator
                        PointOpt::Sep => (),
                    }
                }
                if empty {
                    stats.dac.report_lost_empty(1);
                }
            }

            // Add correction to DAC.
            dac = dac.saturating_add(handle.dac_add.load(Ordering::Acquire));

            stats.dac.update_value(dac);

            // Transfer DAC/ADC values to/from SkifIO board.
            {
                let adcs = match skifio.transfer(XferOut { dac }) {
                    Ok(XferIn { adcs, temp, status }) => {
                        stats.set_skifio_temp(temp);
                        stats.set_skifio_status(status);

                        self.adc.last_point = adcs;
                        adcs
                    }
                    Err(Error {
                        kind: ErrorKind::InvalidData,
                        ..
                    }) => {
                        // CRC check error
                        stats.report_crc_error();
                        self.adc.last_point
                    }
                    Err(e) => panic!("{:?}", e),
                };

                // Handle ADCs
                {
                    #[cfg(feature = "fake")]
                    while !self.adc.buffer.wait(1, BUFFER_TIMEOUT) {
                        println!("ADC buffer timeout");
                    }

                    // Update ADC value statistics
                    stats.adcs.update_values(adcs);
                    // Push ADC point to buffer.
                    if self.adc.buffer.push(adcs.map(Point::from_uv)).is_err() {
                        stats.adcs.report_lost_full(1);
                    }

                    // Increment ADC notification counter.
                    self.adc.counter += 1;
                    if self.adc.counter >= handle.adc_notify_every.load(Ordering::Acquire) {
                        self.adc.counter = 0;
                        ready = true;
                    }
                }
            }

            if ready {
                // Notify
                handle.ready_sem.try_give(cx);
            }

            stats.report_sample();
        }
    }

    pub fn run(self, priority: Priority) {
        task::Builder::new()
            .name("control")
            .priority(priority)
            .spawn(move |cx| self.task_main(cx))
            .unwrap();
    }
}
