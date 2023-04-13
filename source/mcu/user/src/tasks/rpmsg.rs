use super::{control::ControlHandle, stats::Statistics};
use crate::{
    buffers::{AdcConsumer, DacBuffer, DacProducer},
    channel::{Channel, Reader, Writer},
    error::{Error, ErrorKind},
};
use alloc::sync::Arc;
use common::{
    config,
    protocol::{self as proto, AppMsg, McuMsg},
    values::{Dout, Point, Value},
};
use core::{
    sync::atomic::{AtomicBool, AtomicUsize, Ordering},
    time::Duration,
};
use flatty::{flat_vec, portable::le, prelude::NativeCast};
use ringbuf::ring_buffer::RbBase;
use ustd::{
    println,
    task::{self, BlockingContext, Context, Priority, TaskContext},
};

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

    pub fn run(self, read_priority: Priority, write_priority: Priority) {
        task::Builder::new()
            .name("rpmsg_init")
            .priority(read_priority.max(write_priority))
            .spawn(move |cx| {
                let channel = Channel::new(cx).unwrap();
                let (reader, writer) = self.split(channel);
                task::Builder::new()
                    .name("rpmsg")
                    .priority(read_priority)
                    .spawn(move |cx| reader.task_main(cx))
                    .unwrap();
                task::Builder::new()
                    .name("rpmsg")
                    .priority(write_priority)
                    .spawn(move |cx| writer.task_main(cx))
                    .unwrap();
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
    fn task_main(mut self, cx: &mut TaskContext) -> ! {
        let mut channel = self.channel.take().unwrap();
        loop {
            let message = match channel.read_message().map_err(Error::from) {
                Ok(msg) => msg,
                Err(Error {
                    kind: ErrorKind::TimedOut,
                    ..
                }) => {
                    if self.common.is_alive() {
                        println!("Keep-alive timeout reached. RPMSG connection is considered to be dead.");
                        self.disconnect(cx);
                    }
                    continue;
                }
                Err(e) => panic!("{:?}", e),
            };

            use proto::AppMsgRef;
            match message.as_ref() {
                AppMsgRef::KeepAlive => {
                    if !self.common.is_alive() {
                        self.connect(cx);
                    }
                    continue;
                }
                _ => {
                    if !self.common.is_alive() {
                        println!("Error: IOC is not connected");
                    }
                }
            }
            match message.as_ref() {
                AppMsgRef::KeepAlive => unreachable!(),
                AppMsgRef::DoutUpdate { value } => self.set_dout(Dout::from_portable(*value)),
                AppMsgRef::DacState { enable } => {
                    println!("Set DAC state: {:?}", enable);
                    self.control.set_dac_mode(cx, enable.to_native());
                }
                AppMsgRef::DacData { points } => self.write_dac(points),
                AppMsgRef::StatsReset => {
                    println!("Reset stats");
                    self.stats.reset();
                }
            }
        }
    }

    fn connect(&mut self, cx: &mut impl Context) {
        self.common.dac_requested.store(0, Ordering::Release);
        self.control.set_dac_mode(cx, true);
        self.common.alive.store(true, Ordering::Release);
        self.control.notify(cx);
        println!("IOC connected");
    }

    fn disconnect(&mut self, cx: &mut impl Context) {
        self.common.alive.store(false, Ordering::Release);
        self.control.set_dac_mode(cx, false);
        self.stats.report_ioc_drop();
        println!("IOC disconnected");
    }

    fn set_dout(&mut self, value: Dout) {
        self.control.set_dout(value);
    }

    fn write_dac(&mut self, points: &[<Point as Value>::Portable]) {
        // Push received points to ring buffer.
        {
            #[cfg(feature = "fake")]
            assert!(self.buffer.wait(points.len(), crate::buffers::BUFFER_TIMEOUT));

            let count = self.buffer.push_iter(&mut points.iter().copied().map(Point::from_portable));
            if points.len() > count {
                self.stats.dac.report_lost_full(points.len() - count);
            }
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

macro_rules! try_timeout {
    ($res:expr, $ret:expr) => {
        match $res.map_err(Error::from) {
            Ok(msg) => Ok(msg),
            Err(Error {
                kind: ErrorKind::TimedOut,
                ..
            }) => {
                println!("RPMSG buffer allocation timed out");
                #[allow(clippy::unused_unit)]
                return $ret;
            }
            Err(e) => Err(e),
        }
    };
}

impl RpmsgWriter {
    fn task_main(mut self, cx: &mut TaskContext) {
        loop {
            if !self.control.wait_ready(cx, Some(Duration::from_secs(10))) {
                println!("RPMSG send task timed out");
                continue;
            }

            if self.common.is_alive() {
                self.send_din(cx);
                self.send_adcs(cx);
                self.send_dac_request(cx);
            } else {
                self.discard_adcs();
            }
        }
    }

    fn send_din(&mut self, _cx: &mut impl BlockingContext) {
        if let Some(din) = self.control.take_din() {
            try_timeout!(self.channel.new_message(), ())
                .unwrap()
                .emplace(proto::McuMsgInitDinUpdate {
                    value: din.into_portable(),
                })
                .unwrap()
                .write()
                .unwrap();
        }
    }

    fn send_adcs(&mut self, _cx: &mut impl BlockingContext) -> usize {
        let mut total = 0;
        const LEN: usize = proto::ADC_MSG_MAX_POINTS;

        while self.buffer.len() >= LEN {
            let mut msg = try_timeout!(self.channel.new_message(), total)
                .unwrap()
                .emplace(proto::McuMsgInitAdcData { points: flat_vec![] })
                .unwrap();

            let count = if let proto::McuMsgMut::AdcData { points } = msg.as_mut() {
                assert_eq!(points.capacity(), LEN);
                points.extend_from_iter(self.buffer.pop_iter().map(|values| values.map(Point::into_portable)));
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

    fn send_dac_request(&mut self, _cx: &mut impl BlockingContext) {
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
            self.common.dac_requested.fetch_add(count, Ordering::AcqRel);
            try_timeout!(self.channel.new_message(), ())
                .unwrap()
                .emplace(proto::McuMsgInitDacRequest {
                    count: le::U32::from(count as u32),
                })
                .unwrap()
                .write()
                .unwrap();
        }
    }

    fn discard_adcs(&mut self) {
        const LEN: usize = proto::ADC_MSG_MAX_POINTS;
        let len = self.buffer.len();
        self.buffer.skip((len / LEN) * LEN);
    }
}
