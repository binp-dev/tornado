use common::{
    config::ADC_COUNT,
    values::{Din, Dout, Point, Value},
};
use futures::{future::pending, FutureExt};
use mcu::{
    error::{Error, ErrorKind, ErrorSource},
    skifio::{self, DinHandler, SkifioIface, SKIFIO},
};
use std::{
    sync::{atomic::Ordering, Arc, Mutex},
    thread::{park, sleep},
    time::Duration,
};
use tokio::{
    runtime::{self, Runtime},
    select,
    sync::mpsc::{channel, Receiver, Sender},
    task::spawn,
    time::sleep as async_sleep,
};
use ustd::task::InterruptContext;

const ADC_CHAN_CAP: usize = 256;
const DAC_CHAN_CAP: usize = 256;
const DIN_CHAN_CAP: usize = 16;
const DOUT_CHAN_CAP: usize = 16;

pub struct SkifioHandle {
    pub dac: Receiver<Point>,
    pub adcs: Sender<[Point; ADC_COUNT]>,
    pub dout: Receiver<Dout>,
    pub din: Sender<Din>,
}

struct Skifio {
    dac: Sender<Point>,
    dac_enabled: bool,
    adcs: Receiver<[Point; ADC_COUNT]>,
    last_adcs: Option<[Point; ADC_COUNT]>,

    dout: Sender<Dout>,
    last_din: Arc<<Din as Value>::Atomic>,
    din_handler: Arc<Mutex<Option<Box<dyn DinHandler>>>>,

    runtime: Runtime,

    count: usize,
}

impl Skifio {
    fn new() -> (Self, SkifioHandle) {
        let (dac_send, dac_recv) = channel(DAC_CHAN_CAP);
        let (adcs_send, adcs_recv) = channel(ADC_CHAN_CAP);
        let (dout_send, dout_recv) = channel(DOUT_CHAN_CAP);
        let (din_send, din_recv) = channel(DIN_CHAN_CAP);
        let last_din = Arc::new(<Din as Value>::Atomic::default());
        let din_handler = Arc::new(Mutex::new(None::<Box<dyn DinHandler>>));
        {
            let mut recv = din_recv;
            let handler = din_handler.clone();
            let last = last_din.clone();
            spawn(async move {
                loop {
                    let din: Din = match recv.recv().await {
                        Some(x) => x,
                        None => pending().await, // Channel closed
                    };
                    last.store(din.into(), Ordering::Release);
                    if let Some(cb) = &mut *handler.lock().unwrap() {
                        let mut ctx = InterruptContext::new();
                        cb(&mut ctx, din);
                    }
                }
            });
        }
        let runtime = runtime::Builder::new_current_thread()
            .enable_time()
            .build()
            .unwrap();
        (
            Self {
                dac: dac_send,
                dac_enabled: false,
                adcs: adcs_recv,
                last_adcs: None,
                dout: dout_send,
                last_din,
                din_handler,
                runtime,
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
                let adcs = match self.adcs.recv().await {
                    Some(xs) => xs,
                    None => return false,
                };
                self.last_adcs = Some(adcs);
            }
            self.dac.reserve().await.is_ok()
        };
        let fut_timed = async {
            match timeout {
                Some(to) => select! {
                    biased;
                    alive = fut.fuse() => Some(alive),
                    () = async_sleep(to).fuse() => None,
                },
                None => Some(fut.await),
            }
        };
        let ready = match self.runtime.block_on(fut_timed) {
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
            Point::default()
        };
        let adcs = self.last_adcs.take().unwrap();
        self.count += 1;
        self.dac.try_send(dac).unwrap();
        Ok(skifio::XferIn {
            adcs,
            temp: 36,
            status: 0,
        })
    }

    fn write_dout(&mut self, dout: Dout) -> Result<(), Error> {
        self.dout.try_send(dout).unwrap();
        Ok(())
    }

    fn read_din(&mut self) -> Din {
        self.last_din.load(Ordering::Acquire).try_into().unwrap()
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
