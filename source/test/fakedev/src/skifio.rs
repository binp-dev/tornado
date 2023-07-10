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

const AI_CHAN_CAP: usize = 256;
const AO_CHAN_CAP: usize = 256;
const DI_CHAN_CAP: usize = 16;
const DO_CHAN_CAP: usize = 16;

pub struct SkifioHandle {
    pub ao: Receiver<Uv>,
    pub ais: Sender<[Uv; AI_COUNT]>,
    pub do_: Receiver<Do>,
    pub di: Sender<Di>,
}

struct Skifio {
    ao: Sender<Uv>,
    ao_enabled: bool,
    ais: Receiver<[Uv; AI_COUNT]>,
    last_ais: Option<[Uv; AI_COUNT]>,

    do_: Sender<Do>,
    last_di: Arc<AtomicBits>,
    di_handler: Arc<Mutex<Option<Box<dyn DiHandler>>>>,

    runtime: Runtime,

    count: usize,
}

impl Skifio {
    fn new() -> (Self, SkifioHandle) {
        let (ao_send, ao_recv) = channel(AO_CHAN_CAP);
        let (ais_send, ais_recv) = channel(AI_CHAN_CAP);
        let (do_send, do_recv) = channel(DO_CHAN_CAP);
        let (di_send, di_recv) = channel(DI_CHAN_CAP);
        let last_di = Arc::new(AtomicBits::default());
        let di_handler = Arc::new(Mutex::new(None::<Box<dyn DiHandler>>));
        {
            let mut recv = di_recv;
            let handler = di_handler.clone();
            let last = last_di.clone();
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
                ao: ao_send,
                ao_enabled: false,
                ais: ais_recv,
                last_ais: None,
                do_: do_send,
                last_di,
                di_handler,
                runtime,
                count: 0,
            },
            SkifioHandle {
                ao: ao_recv,
                ais: ais_send,
                do_: do_recv,
                di: di_send,
            },
        )
    }
}

impl SkifioIface for Skifio {
    fn set_ao_state(&mut self, enabled: bool) -> Result<(), Error> {
        self.ao_enabled = enabled;
        Ok(())
    }
    fn ao_state(&self) -> bool {
        self.ao_enabled
    }

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        if self.last_ais.is_some() {
            return Ok(());
        }
        let fut = async {
            if self.last_ais.is_none() {
                let adcs = match self.ais.recv().await {
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
        let ao = if self.ao_enabled {
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
