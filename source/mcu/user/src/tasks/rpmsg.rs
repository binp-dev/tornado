use super::{control::ControlHandle, stats::Statistics};
use crate::{
    buffers::{AiConsumer, AoObserver, AoProducer},
    channel::{Channel, Reader, Writer},
    error::{Error, ErrorKind},
    wf,
};
use alloc::sync::Arc;
use common::{
    config,
    protocol::{self as proto, AppMsg, McuMsg},
    values::Point,
};
use core::{
    sync::atomic::{AtomicBool, AtomicUsize, Ordering},
    time::Duration,
};
use flatty::{flat_vec, prelude::*, vec::FromIterator};
use ringbuf::traits::*;
#[cfg(feature = "fake")]
use ringbuf_blocking::traits::*;
use ustd::{
    println,
    task::{self, BlockingContext, Context, Priority, TaskContext},
};

pub struct Rpmsg {
    control: Arc<ControlHandle>,
    stats: Arc<Statistics>,
    ao_buffer: AoProducer,
    ai_buffer: AiConsumer,
    ao_observer: AoObserver,
}

pub struct RpmsgCommon {
    /// Whether IOC is alive.
    alive: AtomicBool,
    /// Number of AO points requested from IOC.
    ao_requested: AtomicUsize,
    ao_observer: AoObserver,
    ping_pong: AtomicBool,
    ping_count: AtomicUsize,
}

pub struct RpmsgReader {
    channel: Option<Reader<AppMsg>>,
    buffer: AoProducer,
    common: Arc<RpmsgCommon>,
    control: Arc<ControlHandle>,
    stats: Arc<Statistics>,
}

pub struct RpmsgWriter {
    channel: Writer<McuMsg>,
    buffer: AiConsumer,
    common: Arc<RpmsgCommon>,
    control: Arc<ControlHandle>,
}

impl Rpmsg {
    pub fn new(control: Arc<ControlHandle>, ao_buffer: AoProducer, ai_buffer: AiConsumer, stats: Arc<Statistics>) -> Self {
        control.configure(proto::AO_MSG_MAX_POINTS, proto::AI_MSG_MAX_POINTS);
        let ao_observer = ao_buffer.observe();
        Self {
            control,
            stats,
            ao_buffer,
            ai_buffer,
            ao_observer,
        }
    }

    fn split(self, channel: Channel) -> (RpmsgReader, RpmsgWriter) {
        let common = Arc::new(RpmsgCommon {
            alive: AtomicBool::new(false),
            ao_requested: AtomicUsize::new(0),
            ao_observer: self.ao_observer,
            ping_pong: AtomicBool::new(false),
            ping_count: AtomicUsize::new(0),
        });
        let (reader, writer) = channel.split();
        (
            RpmsgReader {
                channel: Some(Reader::new(reader, Some(config::KEEP_ALIVE_MAX_DELAY))),
                buffer: self.ao_buffer,
                common: common.clone(),
                control: self.control.clone(),
                stats: self.stats,
            },
            RpmsgWriter {
                channel: Writer::new(writer, None),
                buffer: self.ai_buffer,
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
                let channel = Channel::new(cx, 0).unwrap();
                let (reader, writer) = self.split(channel);
                task::Builder::new()
                    .name("rpmsg_read")
                    .priority(read_priority)
                    .spawn(move |cx| reader.task_main(cx))
                    .unwrap();
                task::Builder::new()
                    .name("rpmsg_write")
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
                Ok(msg) => {
                    if !self.common.is_alive() {
                        self.connect(cx);
                    }
                    msg
                }
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
                AppMsgRef::KeepAlive => {}
                AppMsgRef::DoUpdate { value } => {
                    // println!("Set Do: {:?}", value);
                    self.control.set_do(*value)
                }
                AppMsgRef::AoState { enable } => {
                    println!("Set AO state: {:?}", enable);
                    self.control.set_ao_mode(cx, enable.to_native());
                }
                AppMsgRef::AoData { points } => self.write_ao(points),
                AppMsgRef::AoAdd { value } => self.control.ao_add.store(*value, Ordering::Release),
                AppMsgRef::StatsReset => {
                    println!("Reset stats");
                    self.stats.reset();
                }
                AppMsgRef::WfBufTest { offset, value } => {
                    println!("Read Wf");
                    assert_eq!(*unsafe { wf::read(*offset as usize, value.len()) }, value.as_slice());
                    assert!(!self.common.ping_pong.swap(true, Ordering::SeqCst));
                    self.control.notify(cx);
                }
            }
        }
    }

    fn connect(&mut self, cx: &mut impl Context) {
        self.common.ao_requested.store(0, Ordering::Release);
        self.control.set_ao_mode(cx, true);
        self.common.alive.store(true, Ordering::Release);
        self.control.notify(cx);
        println!("IOC connected");
    }

    fn disconnect(&mut self, cx: &mut impl Context) {
        self.common.alive.store(false, Ordering::Release);
        self.control.set_ao_mode(cx, false);
        self.stats.report_ioc_drop();
        println!("IOC disconnected");
    }

    fn write_ao(&mut self, points: &[Point]) {
        // Push received points to ring buffer.
        {
            #[cfg(feature = "fake")]
            assert!(self.buffer.wait_vacant(points.len(), crate::buffers::BUFFER_TIMEOUT));

            let count = self.buffer.push_iter(&mut points.iter().copied());
            if points.len() > count {
                self.stats.ao.report_lost_full(points.len() - count);
            }
        }

        // Safely decrement requested points counter.
        {
            let mut len = points.len();
            let req = self.common.ao_requested.load(Ordering::Acquire);
            if req < len {
                self.stats.ao.report_req_exceed(len - req);
                len = req;
            }
            self.common.ao_requested.fetch_sub(len, Ordering::AcqRel);
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
                self.send_di(cx);
                self.send_ais(cx);
                self.send_ao_request(cx);
                self.send_wf_test(cx);
            } else {
                self.discard_ais();
            }
        }
    }

    fn send_di(&mut self, _cx: &mut impl BlockingContext) {
        if let Some(value) = self.control.take_di() {
            try_timeout!(self.channel.alloc_message(), ())
                .unwrap()
                .new_in_place(proto::McuMsgInitDiUpdate { value })
                .unwrap()
                .write()
                .unwrap();
        }
    }

    fn send_ais(&mut self, _cx: &mut impl BlockingContext) -> usize {
        let mut total = 0;
        const LEN: usize = proto::AI_MSG_MAX_POINTS;

        while self.buffer.occupied_len() >= LEN {
            let mut msg = try_timeout!(self.channel.alloc_message(), total)
                .unwrap()
                .new_in_place(proto::McuMsgInitAiData { points: flat_vec![] })
                .unwrap();

            let count = if let proto::McuMsgMut::AiData { points } = msg.as_mut() {
                assert_eq!(points.capacity(), LEN);
                points.extend_from_iter(self.buffer.pop_iter());
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

    fn send_ao_request(&mut self, _cx: &mut impl BlockingContext) {
        const SIZE: usize = proto::AO_MSG_MAX_POINTS;
        let vacant = self.common.ao_observer.vacant_len();
        let requested = self.common.ao_requested.load(Ordering::Acquire);
        let mut raw_count = 0;
        if requested <= vacant {
            raw_count = vacant - requested;
        }
        if raw_count >= SIZE {
            // Request number of points that is multiple of `AO_MSG_MAX_POINTS`.
            let count = (raw_count / SIZE) * SIZE;
            self.common.ao_requested.fetch_add(count, Ordering::AcqRel);
            try_timeout!(self.channel.alloc_message(), ())
                .unwrap()
                .new_in_place(proto::McuMsgInitAoRequest { count: count as u32 })
                .unwrap()
                .write()
                .unwrap();
        }
    }

    fn discard_ais(&mut self) {
        const LEN: usize = proto::AI_MSG_MAX_POINTS;
        let len = self.buffer.occupied_len();
        self.buffer.skip((len / LEN) * LEN);
    }

    fn send_wf_test(&mut self, _cx: &mut impl BlockingContext) {
        if self.common.ping_pong.swap(false, Ordering::SeqCst) {
            let j = self.common.ping_count.fetch_add(1, Ordering::SeqCst);
            println!("Write Wf: {}", j);

            let offset = 0;
            let len = 4 * wf::offset_align();
            let mut data = unsafe { wf::write(offset, len) };
            for (i, b) in data.iter_mut().enumerate() {
                *b = ((j + i) % 256) as u8;
            }
            let msg = try_timeout!(self.channel.alloc_message(), ())
                .unwrap()
                .new_in_place(proto::McuMsgInitWfBufTest {
                    offset: offset as u32,
                    value: FromIterator(data.iter().copied()),
                })
                .unwrap();
            drop(data);
            msg.write().unwrap();
        }
    }
}
