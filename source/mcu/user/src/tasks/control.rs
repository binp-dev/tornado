use super::stats::Statistics;
#[cfg(feature = "real")]
use crate::skifio::SkifioIface as _;
use crate::{
    buffers::{AdcProducer, DacConsumer},
    error::{Error, ErrorKind},
    println,
    skifio::{self, AtomicDin, AtomicDout, DinHandler, XferIn, XferOut},
};
use alloc::{boxed::Box, sync::Arc};
use common::{
    config::ADC_COUNT,
    units::{AdcPoint, DacPoint, Unit},
};
use core::{
    sync::atomic::{AtomicBool, AtomicUsize, Ordering},
    time::Duration,
};
use ustd::{sync::Semaphore, task};
use ux::u4;

pub type Din = u8;
pub type Dout = u4;

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

    /// Number of DAC points to write until notified.
    dac_notify_every: AtomicUsize,
    /// Number of ADC points to read until notified.
    adc_notify_every: AtomicUsize,
}

struct ControlDac {
    buffer: DacConsumer,
    last_point: DacPoint,
    counter: usize,
}

struct ControlAdc {
    buffer: AdcProducer,
    last_point: [AdcPoint; ADC_COUNT],
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
            din: AtomicDin::new(0),
            dout: AtomicDout::new(0),
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

    pub fn notify(&self) {
        self.ready_sem.try_give();
    }
    pub fn wait_ready(&self, timeout: Option<Duration>) -> bool {
        match timeout {
            Some(to) => self.ready_sem.take_timeout(to),
            None => {
                self.ready_sem.take();
                true
            }
        }
    }

    pub fn set_dac_mode(&self, enabled: bool) {
        self.dac_enabled.store(enabled, Ordering::Release);
    }

    fn update_din(&self, value: Din) -> bool {
        if self.din.swap(value, Ordering::AcqRel) != value {
            self.din_changed.fetch_or(true, Ordering::AcqRel);
            true
        } else {
            false
        }
    }
    pub fn take_din(&self) -> Option<Din> {
        if self.din_changed.fetch_and(false, Ordering::AcqRel) {
            Some(self.din.load(Ordering::Acquire))
        } else {
            None
        }
    }

    pub fn set_dout(&self, value: Dout) {
        let raw = value.into();
        if self.dout.swap(raw, Ordering::AcqRel) != raw {
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
                    last_point: DacPoint::ZERO,
                    counter: 0,
                },
                adc: ControlAdc {
                    buffer: adc_buf,
                    last_point: [AdcPoint::default(); ADC_COUNT],
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
        Box::new(move |context, din| {
            if handle.update_din(din) {
                handle.ready_sem.try_give_from_intr(context);
            }
        })
    }

    fn task_main(mut self) -> ! {
        let handle = self.handle.clone();
        let stats = self.stats.clone();

        let mut skifio = skifio::handle().unwrap();
        skifio.subscribe_din(Some(self.make_din_handler())).unwrap();

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
                skifio.write_dout(handle.dout.load(Ordering::Acquire)).unwrap();
            }

            // Read discrete input
            ready |= handle.update_din(skifio.read_din());

            // Fetch next DAC value from buffer
            let mut dac = self.dac.last_point;
            if handle.dac_enabled.load(Ordering::Acquire) {
                if let Some(value) = self.dac.buffer.pop() {
                    dac = value;
                    self.dac.last_point = value;
                    // Increment DAC notification counter.
                    self.dac.counter += 1;
                    if self.dac.counter >= handle.dac_notify_every.load(Ordering::Acquire) {
                        self.dac.counter = 0;
                        ready = true;
                    }
                } else {
                    stats.dac.report_lost_empty(1);
                }
                stats.dac.update_value(dac);
            }

            // Transfer DAC/ADC values to/from SkifIO board.
            {
                // TODO: Check for overflow.
                let adcs = match skifio.transfer(XferOut { dac }) {
                    Ok(XferIn { adcs }) => {
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
                    // Update ADC value statistics
                    stats.adcs.update_values(adcs);
                    // Push ADC point to buffer.
                    if self.adc.buffer.push(adcs).is_err() {
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
                handle.ready_sem.try_give();
            }

            stats.report_sample();
        }
    }

    pub fn run(self, priority: usize) {
        task::spawn(task::Priority(priority), move || self.task_main()).unwrap();
    }
}
