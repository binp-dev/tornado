use async_std::task::{sleep as async_sleep, spawn};
use common::config::{self, ADC_COUNT};
use futures::{
    channel::mpsc::{
        unbounded as channel, UnboundedReceiver as Receiver, UnboundedSender as Sender,
    },
    executor::block_on,
    future::pending,
    select_biased, FutureExt, StreamExt,
};
use mcu::{
    error::{Error, ErrorKind, ErrorSource},
    skifio::{self, Ain, Aout, AtomicDin, Din, DinHandler, Dout, SkifioIface, SKIFIO},
};
use std::{
    sync::{Arc, Mutex},
    thread::{park, sleep},
    time::Duration,
};
use ustd::interrupt::InterruptContext;

type Ains = [Ain; ADC_COUNT];

pub struct SkifioHandle {
    pub dac: Receiver<Aout>,
    pub adcs: Sender<Ains>,
    pub dout: Receiver<Dout>,
    pub din: Sender<Din>,
}

struct Skifio {
    dac: Sender<Aout>,
    dac_enabled: bool,
    adcs: Receiver<Ains>,
    last_adcs: Option<Ains>,

    dout: Sender<Dout>,
    last_din: Arc<AtomicDin>,
    din_handler: Arc<Mutex<Option<Box<dyn DinHandler>>>>,

    count: usize,
}

impl Skifio {
    fn new() -> (Self, SkifioHandle) {
        let (dac_send, dac_recv) = channel();
        let (adcs_send, adcs_recv) = channel();
        let (dout_send, dout_recv) = channel();
        let (din_send, din_recv) = channel();
        let last_din = Arc::new(AtomicDin::default());
        let din_handler = Arc::new(Mutex::new(None::<Box<dyn DinHandler>>));
        {
            let mut recv = din_recv;
            let handler = din_handler.clone();
            let last = last_din.clone();
            spawn(async move {
                loop {
                    let din = match recv.next().await {
                        Some(x) => x,
                        None => pending().await, // Channel closed
                    };
                    last.store(din, std::sync::atomic::Ordering::Release);
                    if let Some(cb) = &mut *handler.lock().unwrap() {
                        let mut ctx = InterruptContext::new();
                        cb(&mut ctx, din);
                    }
                }
            });
        }
        (
            Self {
                dac: dac_send,
                dac_enabled: false,
                adcs: adcs_recv,
                last_adcs: None,
                dout: dout_send,
                last_din,
                din_handler,
                count: 0,
            },
            SkifioHandle {
                dac: dac_recv,
                adcs: adcs_send,
                dout: dout_recv,
                din: din_send,
            },
        )
    }
}

impl SkifioIface for Skifio {
    fn set_dac_state(&mut self, enabled: bool) -> Result<(), Error> {
        self.dac_enabled = enabled;
        Ok(())
    }
    fn dac_state(&self) -> bool {
        self.dac_enabled
    }

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        if self.last_adcs.is_some() {
            return Ok(());
        }
        let fut = async {
            match timeout {
                Some(to) => select_biased! {
                    xs = self.adcs.next().fuse() => Some(xs),
                    () = async_sleep(to).fuse() => None,
                },
                None => Some(self.adcs.next().await),
            }
        };
        let res = block_on(fut);
        let res = match res {
            Some(Some(xs)) => Some(xs),
            Some(None) => {
                // ADC channel closed
                match timeout {
                    Some(to) => {
                        sleep(to);
                        None
                    }
                    None => loop {
                        park();
                    },
                }
            }
            None => None,
        };
        match res {
            Some(xs) => {
                assert!(self.last_adcs.replace(xs).is_none());
                Ok(())
            }
            None => Err(Error {
                kind: ErrorKind::TimedOut,
                source: ErrorSource::None,
            }),
        }
    }
    fn transfer(&mut self, out: skifio::XferOut) -> Result<skifio::XferIn, Error> {
        assert!(self.last_adcs.is_some());
        let dac = if self.dac_enabled {
            out.dac
        } else {
            config::DAC_RAW_OFFSET as Aout
        };
        let adcs = self.last_adcs.take().unwrap();
        self.count += 1;
        self.dac.unbounded_send(dac).unwrap();
        Ok(skifio::XferIn { adcs })
    }

    fn write_dout(&mut self, dout: Dout) -> Result<(), Error> {
        self.dout.unbounded_send(dout).unwrap();
        Ok(())
    }

    fn read_din(&mut self) -> Din {
        self.last_din.load(std::sync::atomic::Ordering::Acquire)
    }
    fn subscribe_din(&mut self, callback: Option<Box<dyn DinHandler>>) -> Result<(), Error> {
        *self.din_handler.lock().unwrap() = callback;
        Ok(())
    }
}

pub fn bind() -> SkifioHandle {
    let (skifio, handle) = Skifio::new();
    assert!(SKIFIO.lock().unwrap().replace(Box::new(skifio)).is_none());
    handle
}
