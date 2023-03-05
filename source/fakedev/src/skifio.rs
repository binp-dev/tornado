use async_std::task::{sleep as async_sleep, spawn};
use common::config::{self, ADC_COUNT};
use futures::{
    channel::mpsc::{channel, Receiver, Sender},
    executor::block_on,
    future::pending,
    select_biased, FutureExt, StreamExt,
};
use mcu::{
    error::{Error, ErrorKind, ErrorSource},
    skifio::{self, Ain, Aout, AtomicDin, Din, DinHandler, Dout, SkifioIface, SKIFIO},
};
use std::{
    future::Future,
    pin::Pin,
    sync::{Arc, Mutex},
    task::{Context, Poll},
    thread::{park, sleep},
    time::Duration,
};
use ustd::interrupt::InterruptContext;

type Ains = [Ain; ADC_COUNT];

const ADC_CHAN_CAP: usize = 256;
const DAC_CHAN_CAP: usize = 256;
const DIN_CHAN_CAP: usize = 16;
const DOUT_CHAN_CAP: usize = 16;

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
        let (dac_send, dac_recv) = channel(DAC_CHAN_CAP);
        let (adcs_send, adcs_recv) = channel(ADC_CHAN_CAP);
        let (dout_send, dout_recv) = channel(DOUT_CHAN_CAP);
        let (din_send, din_recv) = channel(DIN_CHAN_CAP);
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
            if self.last_adcs.is_none() {
                let adcs = match self.adcs.next().await {
                    Some(xs) => xs,
                    None => return false,
                };
                self.last_adcs = Some(adcs);
            }
            WaitReady(&mut self.dac).await
        };
        let fut_timed = async {
            match timeout {
                Some(to) => select_biased! {
                    alive = fut.fuse() => Some(alive),
                    () = async_sleep(to).fuse() => None,
                },
                None => Some(fut.await),
            }
        };
        let ready = match block_on(fut_timed) {
            Some(alive) => {
                if alive {
                    true
                } else {
                    // ADC channel closed
                    match timeout {
                        Some(to) => {
                            sleep(to);
                            false
                        }
                        None => loop {
                            park();
                        },
                    }
                }
            }
            None => false,
        };
        if ready {
            Ok(())
        } else {
            Err(Error {
                kind: ErrorKind::TimedOut,
                source: ErrorSource::None,
            })
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
        if self.count % 1000 == 0 {
            sleep(Duration::from_millis(1));
        }
        self.dac.try_send(dac).unwrap();
        Ok(skifio::XferIn { adcs })
    }

    fn write_dout(&mut self, dout: Dout) -> Result<(), Error> {
        self.dout.try_send(dout).unwrap();
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

struct WaitReady<'a, T: Send + 'static>(&'a mut Sender<T>);
impl<'a, T: Send + 'static> Future for WaitReady<'a, T> {
    type Output = bool;
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        self.0.poll_ready(cx).map(|r| r.is_ok())
    }
}
