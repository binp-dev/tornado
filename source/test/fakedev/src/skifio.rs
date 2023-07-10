use common::{
    config::AI_COUNT,
    values::{AtomicBits, Di, Do, Uv},
};
use futures::{future::pending, FutureExt};
use mcu::{
    error::{Error, ErrorKind, ErrorSource},
    skifio::{self, DiHandler, SkifioIface, SKIFIO},
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
    pub dac: Receiver<Uv>,
    pub adcs: Sender<[Uv; AI_COUNT]>,
    pub dout: Receiver<Do>,
    pub din: Sender<Di>,
}

struct Skifio {
    ao: Sender<Uv>,
    dac_enabled: bool,
    adcs: Receiver<[Uv; AI_COUNT]>,
    last_ais: Option<[Uv; AI_COUNT]>,

    do_: Sender<Do>,
    last_di: Arc<AtomicBits>,
    di_handler: Arc<Mutex<Option<Box<dyn DiHandler>>>>,

    runtime: Runtime,

    count: usize,
}

impl Skifio {
    fn new() -> (Self, SkifioHandle) {
        let (dac_send, dac_recv) = channel(DAC_CHAN_CAP);
        let (adcs_send, adcs_recv) = channel(ADC_CHAN_CAP);
        let (dout_send, dout_recv) = channel(DOUT_CHAN_CAP);
        let (din_send, din_recv) = channel(DIN_CHAN_CAP);
        let last_din = Arc::new(AtomicBits::default());
        let din_handler = Arc::new(Mutex::new(None::<Box<dyn DiHandler>>));
        {
            let mut recv = din_recv;
            let handler = din_handler.clone();
            let last = last_din.clone();
            spawn(async move {
                loop {
                    let din: Di = match recv.recv().await {
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
                ao: dac_send,
                dac_enabled: false,
                adcs: adcs_recv,
                last_ais: None,
                do_: dout_send,
                last_di: last_din,
                di_handler: din_handler,
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
    fn set_ao_state(&mut self, enabled: bool) -> Result<(), Error> {
        self.dac_enabled = enabled;
        Ok(())
    }
    fn ao_state(&self) -> bool {
        self.dac_enabled
    }

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        if self.last_ais.is_some() {
            return Ok(());
        }
        let fut = async {
            if self.last_ais.is_none() {
                let adcs = match self.adcs.recv().await {
                    Some(xs) => xs,
                    None => return false,
                };
                self.last_ais = Some(adcs);
            }
            self.ao.reserve().await.is_ok()
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
        assert!(self.last_ais.is_some());
        let ao = if self.dac_enabled {
            out.ao
        } else {
            Uv::default()
        };
        let ais = self.last_ais.take().unwrap();
        self.count += 1;
        self.ao.try_send(ao).unwrap();
        Ok(skifio::XferIn {
            ais,
            temp: 36,
            status: 0,
        })
    }

    fn write_do(&mut self, do_: Do) -> Result<(), Error> {
        self.do_.try_send(do_).unwrap();
        Ok(())
    }

    fn read_di(&mut self) -> Di {
        self.last_di.load(Ordering::Acquire).try_into().unwrap()
    }
    fn subscribe_di(&mut self, callback: Option<Box<dyn DiHandler>>) -> Result<(), Error> {
        *self.di_handler.lock().unwrap() = callback;
        Ok(())
    }
}

pub fn bind() -> SkifioHandle {
    let (skifio, handle) = Skifio::new();
    assert!(SKIFIO.lock().unwrap().replace(Box::new(skifio)).is_none());
    handle
}
