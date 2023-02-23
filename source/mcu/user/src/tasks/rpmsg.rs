use super::{control::ControlHandle, stats::Statistics};
use crate::{
    buffers::{AdcConsumer, DacBuffer, DacProducer},
    channel::{Channel, Reader, Writer},
    error::ErrorKind,
    Error,
};
use alloc::sync::Arc;
use common::{
    config::{self, PointPortable, DAC_COUNT},
    protocol::{self as proto, AppMsg, McuMsg},
};
use core::{
    sync::atomic::{AtomicBool, AtomicUsize, Ordering},
    time::Duration,
};
use flatty::{flat_vec, portable::le, prelude::*};
use ringbuf::ring_buffer::RbBase;
use ustd::{prelude::*, task};

pub struct Rpmsg {
    control: Arc<ControlHandle>,
    stats: Arc<Statistics>,
    dac_buffer: DacProducer,
    adc_buffer: AdcConsumer,
    dac_observer: &'static DacBuffer,
}

pub struct RpmsgCommon {
    /// Whether IOC is alive.
    alive: AtomicBool,
    /// Number of DAC points requested from IOC.
    dac_requested: AtomicUsize,

    dac_observer: &'static DacBuffer,
}

pub struct RpmsgReader {
    channel: Option<Reader<AppMsg>>,
    buffer: DacProducer,
    common: Arc<RpmsgCommon>,
    control: Arc<ControlHandle>,
    stats: Arc<Statistics>,
}

pub struct RpmsgWriter {
    channel: Writer<McuMsg>,
    buffer: AdcConsumer,
    common: Arc<RpmsgCommon>,
    control: Arc<ControlHandle>,
}

impl Rpmsg {
    pub fn new(
        control: Arc<ControlHandle>,
        dac_buffer: DacProducer,
        adc_buffer: AdcConsumer,
        dac_observer: &'static DacBuffer,
        stats: Arc<Statistics>,
    ) -> Self {
        control.configure(proto::DAC_MSG_MAX_POINTS, proto::ADC_MSG_MAX_POINTS);
        Self {
            control,
            stats,
            dac_buffer,
            adc_buffer,
            dac_observer,
        }
    }

    fn split(self, channel: Channel) -> (RpmsgReader, RpmsgWriter) {
        let common = Arc::new(RpmsgCommon {
            alive: AtomicBool::new(false),
            dac_requested: AtomicUsize::new(0),
            dac_observer: self.dac_observer,
        });
        let (reader, writer) = channel.split();
        (
            RpmsgReader {
                channel: Some(Reader::new(reader, Some(config::KEEP_ALIVE_MAX_DELAY))),
                buffer: self.dac_buffer,
                common: common.clone(),
                control: self.control.clone(),
                stats: self.stats,
            },
            RpmsgWriter {
                channel: Writer::new(writer, None),
                buffer: self.adc_buffer,
                common,
                control: self.control,
            },
        )
    }

    pub fn run(self, read_priority: usize, write_priority: usize) {
        task::spawn(task::Priority(read_priority.max(write_priority)), move || {
            let channel = Channel::new(&task::current().unwrap()).unwrap();
            let (reader, writer) = self.split(channel);
            task::spawn(task::Priority(read_priority), move || reader.task_main()).unwrap();
            task::spawn(task::Priority(write_priority), move || writer.task_main()).unwrap();
        })
        .unwrap();
    }
}

impl RpmsgCommon {
    fn is_alive(&self) -> bool {
        self.alive.load(Ordering::Acquire)
    }
}

impl RpmsgReader {
    fn task_main(mut self) -> ! {
        let mut channel = self.channel.take().unwrap();
        loop {
            let message = match channel.read_message().map_err(|e| Error::from(e)) {
                Ok(msg) => msg,
                Err(Error {
                    kind: ErrorKind::TimedOut,
                    ..
                }) => {
                    if self.common.is_alive() {
                        println!("Keep-alive timeout reached. RPMSG connection is considered to be dead.");
                        self.disconnect();
                    }
                    continue;
                }
                Err(e) => panic!("{:?}", e),
            };

            use proto::AppMsgRef;
            match message.as_ref() {
                AppMsgRef::Connect => {
                    self.connect();
                    continue;
                }
                _ => {
                    if !self.common.is_alive() {
                        println!("Error: IOC is not connected");
                    }
                }
            }
            match message.as_ref() {
                AppMsgRef::Connect => unreachable!(),
                AppMsgRef::KeepAlive => continue,
                AppMsgRef::DoutUpdate { value } => self.set_dout(*value),
                AppMsgRef::DacState { enable } => {
                    println!("Set DAC state: {}", enable);
                    self.control.set_dac_mode(*enable != 0);
                }
                AppMsgRef::DacData { points } => self.write_dac(points),
                AppMsgRef::StatsReset => {
                    println!("Reset stats");
                    self.stats.reset();
                }
            }
        }
    }

    fn connect(&mut self) {
        self.common.dac_requested.store(0, Ordering::Release);
        self.control.set_dac_mode(true);
        self.common.alive.store(true, Ordering::Release);
        self.control.notify();
        println!("IOC connected");
    }

    fn disconnect(&mut self) {
        self.common.alive.store(false, Ordering::Release);
        self.control.set_dac_mode(false);
        println!("IOC disconnected");
    }

    fn set_dout(&mut self, value: u8) {
        match value.try_into() {
            Ok(x) => self.control.set_dout(x),
            Err(_) => println!("Dout is out of bounds: {:b}", value),
        }
    }

    fn write_dac(&mut self, points: &[[PointPortable; DAC_COUNT]]) {
        {
            let count = self.buffer.push_iter(&mut points.iter().map(|[p]| p.to_native()));
            self.stats.dac.report_lost_full(points.len() - count);
        }

        // Safely decrement requested points counter.
        {
            let mut len = points.len();
            let req = self.common.dac_requested.load(Ordering::Acquire);
            if req < len {
                self.stats.dac.report_req_exceed(len - req);
                len = req;
            }
            self.common.dac_requested.fetch_sub(len, Ordering::AcqRel);
        }
    }
}

impl RpmsgWriter {
    fn task_main(mut self) {
        loop {
            if !self.control.wait_ready(Some(Duration::from_secs(10))) {
                println!("RPMSG send task timed out");
                continue;
            }

            if self.common.is_alive() {
                self.send_din();
                self.send_adcs();
                self.send_dac_request();
            } else {
                self.discard_adcs();
            }
        }
    }

    fn send_din(&mut self) {
        if let Some(din) = self.control.take_din() {
            self.channel
                .new_message()
                .emplace(proto::McuMsgInitDinUpdate { value: din })
                .unwrap()
                .write()
                .unwrap();
        }
    }

    fn send_adcs(&mut self) -> usize {
        let mut total = 0;
        const LEN: usize = proto::ADC_MSG_MAX_POINTS;

        while self.buffer.len() >= LEN {
            let mut msg = self
                .channel
                .new_message()
                .emplace(proto::McuMsgInitAdcData { points: flat_vec![] })
                .unwrap();

            let count = if let proto::McuMsgMut::AdcData { points } = msg.as_mut() {
                assert_eq!(points.capacity(), LEN);
                points.extend_from_iter(self.buffer.pop_iter().map(|values| values.map(PointPortable::from)));
                points.len()
            } else {
                unreachable!()
            };

            assert_eq!(count, LEN);
            msg.write().unwrap();
            total += count;
        }
        total
    }

    fn send_dac_request(&mut self) {
        const SIZE: usize = proto::DAC_MSG_MAX_POINTS;
        let vacant = self.common.dac_observer.vacant_len();
        let requested = self.common.dac_requested.load(Ordering::Acquire);
        let mut raw_count = 0;
        if requested <= vacant {
            raw_count = vacant - requested;
        }
        if raw_count >= SIZE {
            // Request number of points that is multiple of `DAC_MSG_MAX_POINTS`.
            let count = (raw_count / SIZE) * SIZE;
            self.channel
                .new_message()
                .emplace(proto::McuMsgInitDacRequest {
                    count: le::U32::from(count as u32),
                })
                .unwrap()
                .write()
                .unwrap();
            self.common.dac_requested.fetch_add(count, Ordering::AcqRel);
        }
    }

    fn discard_adcs(&mut self) {
        const LEN: usize = proto::ADC_MSG_MAX_POINTS;
        self.buffer.skip((self.buffer.len() / LEN) * LEN);
    }
}
