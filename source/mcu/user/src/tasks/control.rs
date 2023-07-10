use super::stats::Statistics;
#[cfg(feature = "fake")]
use crate::buffers::BUFFER_TIMEOUT;
#[cfg(feature = "real")]
use crate::skifio::SkifioIface as _;
use crate::{
    buffers::{AiProducer, AoConsumer},
    error::{Error, ErrorKind},
    println,
    skifio::{self, DiHandler, XferIn, XferOut},
};
use alloc::{boxed::Box, sync::Arc};
use common::{
    config::AI_COUNT,
    values::{AtomicBits, AtomicUv, Di, Do, Point, PointOpt, Uv},
};
use core::{
    sync::atomic::{AtomicBool, AtomicUsize, Ordering},
    time::Duration,
};
use ringbuf::traits::*;
#[cfg(feature = "fake")]
use ringbuf_blocking::traits::*;
use ustd::{
    sync::Semaphore,
    task::{self, BlockingContext, Context, Priority, TaskContext},
};

pub struct ControlHandle {
    /// Semaphore to notify that something is ready.
    ready_sem: Semaphore,

    ao_enabled: AtomicBool,
    #[cfg(feature = "fake")]
    ao_enable_sem: Semaphore,

    pub ao_add: AtomicUv,

    di: AtomicBits,
    pub do_: AtomicBits,

    /// Discrete input has changed.
    di_changed: AtomicBool,
    /// Discrete output has changed.
    do_changed: AtomicBool,

    /// Number of AO points to write until notified.
    ao_notify_every: AtomicUsize,
    /// Number of AI points to read until notified.
    ai_notify_every: AtomicUsize,
}

struct ControlAo {
    buffer: AoConsumer,
    last_point: Uv,
    counter: usize,
}

struct ControlAi {
    buffer: AiProducer,
    last_point: [Uv; AI_COUNT],
    counter: usize,
}

pub struct Control {
    ao: ControlAo,
    ai: ControlAi,
    handle: Arc<ControlHandle>,
    stats: Arc<Statistics>,
}

impl ControlHandle {
    fn new() -> Self {
        Self {
            ready_sem: Semaphore::new().unwrap(),
            ao_enabled: AtomicBool::new(false),
            #[cfg(feature = "fake")]
            ao_enable_sem: Semaphore::new().unwrap(),
            ao_add: AtomicUv::default(),
            di: AtomicBits::default(),
            do_: AtomicBits::default(),
            di_changed: AtomicBool::new(false),
            do_changed: AtomicBool::new(false),
            ao_notify_every: AtomicUsize::new(0),
            ai_notify_every: AtomicUsize::new(0),
        }
    }
    pub fn configure(&self, ao_notify_every: usize, ai_notify_every: usize) {
        self.ao_notify_every.store(ao_notify_every, Ordering::Release);
        self.ai_notify_every.store(ai_notify_every, Ordering::Release);
    }

    pub fn notify(&self, cx: &mut impl Context) {
        self.ready_sem.try_give(cx);
    }
    pub fn wait_ready(&self, cx: &mut impl BlockingContext, timeout: Option<Duration>) -> bool {
        self.ready_sem.take(cx, timeout)
    }

    pub fn set_ao_mode(&self, _cx: &mut impl Context, enabled: bool) {
        self.ao_enabled.store(enabled, Ordering::Release);
        #[cfg(feature = "fake")]
        if enabled {
            self.ao_enable_sem.try_give(_cx);
        }
    }

    fn update_di(&self, value: Di) -> bool {
        if self.di.swap(value.into(), Ordering::AcqRel) != value.into() {
            self.di_changed.fetch_or(true, Ordering::AcqRel);
            true
        } else {
            false
        }
    }
    pub fn take_di(&self) -> Option<Di> {
        if self.di_changed.fetch_and(false, Ordering::AcqRel) {
            Some(self.di.load(Ordering::Acquire).try_into().unwrap())
        } else {
            None
        }
    }

    pub fn set_do(&self, value: Do) {
        if self.do_.swap(value.into(), Ordering::AcqRel) != value.into() {
            self.do_changed.fetch_or(true, Ordering::AcqRel);
        }
    }
}

impl Control {
    pub fn new(ao_buf: AoConsumer, ai_buf: AiProducer, stats: Arc<Statistics>) -> (Self, Arc<ControlHandle>) {
        let handle = Arc::new(ControlHandle::new());
        (
            Self {
                ao: ControlAo {
                    buffer: ao_buf,
                    last_point: Uv::default(),
                    counter: 0,
                },
                ai: ControlAi {
                    buffer: ai_buf,
                    last_point: [Uv::default(); AI_COUNT],
                    counter: 0,
                },
                handle: handle.clone(),
                stats,
            },
            handle,
        )
    }

    fn make_di_handler(&self) -> Box<dyn DiHandler> {
        let handle = self.handle.clone();
        Box::new(move |cx, di| {
            if handle.update_di(di) {
                handle.ready_sem.try_give(cx);
            }
        })
    }

    fn task_main(mut self, cx: &mut TaskContext) -> ! {
        let handle = self.handle.clone();
        let stats = self.stats.clone();

        let mut skifio = skifio::handle().unwrap();
        skifio.subscribe_di(Some(self.make_di_handler())).unwrap();

        #[cfg(feature = "fake")]
        while !handle.ao_enabled.load(Ordering::Acquire) {
            if !handle.ao_enable_sem.take(cx, BUFFER_TIMEOUT) {
                println!("AO enable timeout");
            }
        }

        println!("Enter SkifIO loop");
        loop {
            let mut ready = false;

            skifio.set_ao_state(handle.ao_enabled.load(Ordering::Acquire)).unwrap();

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
            if handle.do_changed.fetch_and(false, Ordering::AcqRel) {
                skifio
                    .write_do(handle.do_.load(Ordering::Acquire).try_into().unwrap())
                    .unwrap();
            }

            // Read discrete input
            ready |= handle.update_di(skifio.read_di());

            // Fetch next AO value from buffer
            let mut ao = self.ao.last_point;
            if handle.ao_enabled.load(Ordering::Acquire) {
                #[cfg(feature = "fake")]
                while !self.ao.buffer.wait_occupied(1, BUFFER_TIMEOUT) {
                    println!("AO buffer timeout");
                }

                let mut empty = true;
                while let Some(p) = self.ao.buffer.try_pop() {
                    match p.into_opt() {
                        PointOpt::Uv(value) => {
                            ao = value;
                            self.ao.last_point = value;
                            // Increment AO notification counter.
                            self.ao.counter += 1;
                            if self.ao.counter >= handle.ao_notify_every.load(Ordering::Acquire) {
                                self.ao.counter = 0;
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
                    stats.ao.report_lost_empty(1);
                }
            }

            // Add correction to AO.
            ao = ao.saturating_add(handle.ao_add.load(Ordering::Acquire));

            stats.ao.update_value(ao);

            // Transfer AO/AI values to/from SkifIO board.
            {
                let ais = match skifio.transfer(XferOut { ao }) {
                    Ok(XferIn { ais, temp, status }) => {
                        stats.set_skifio_temp(temp);
                        stats.set_skifio_status(status);

                        self.ai.last_point = ais;
                        ais
                    }
                    Err(Error {
                        kind: ErrorKind::InvalidData,
                        ..
                    }) => {
                        // CRC check error
                        stats.report_crc_error();
                        self.ai.last_point
                    }
                    Err(e) => panic!("{:?}", e),
                };

                // Handle AIs
                {
                    #[cfg(feature = "fake")]
                    while !self.ai.buffer.wait_vacant(1, BUFFER_TIMEOUT) {
                        println!("AI buffer timeout");
                    }

                    // Update AI value statistics
                    stats.ais.update_values(ais);
                    // Push AI point to buffer.
                    if self.ai.buffer.try_push(ais.map(Point::from_uv)).is_err() {
                        stats.ais.report_lost_full(1);
                    }

                    // Increment AI notification counter.
                    self.ai.counter += 1;
                    if self.ai.counter >= handle.ai_notify_every.load(Ordering::Acquire) {
                        self.ai.counter = 0;
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
