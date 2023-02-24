use async_std::task::{sleep, spawn};
use common::config::ADC_COUNT;
use futures::{
    channel::mpsc::{
        unbounded as channel, UnboundedReceiver as Receiver, UnboundedSender as Sender,
    },
    executor::block_on,
    select_biased, FutureExt, StreamExt,
};
use mcu::{
    error::{Error, ErrorKind, ErrorSource},
    skifio::{self, AtomicDin, SkifioIface, SKIFIO},
};
use std::{
    sync::{Arc, Mutex},
    time::Duration,
};
use ustd::interrupt::InterruptContext;

type Ains = [skifio::Ain; ADC_COUNT];

pub struct SkifioHandle {
    pub dac: Receiver<skifio::Aout>,
    pub adcs: Sender<Ains>,
    pub dout: Receiver<skifio::Dout>,
    pub din: Sender<skifio::Din>,
}

struct Skifio {
    dac: Sender<skifio::Aout>,
    adcs: Receiver<Ains>,
    last_adcs: Option<Ains>,

    dout: Sender<skifio::Dout>,
    last_din: Arc<skifio::AtomicDin>,
    din_handler: Arc<Mutex<Option<Box<dyn skifio::DinHandler>>>>,
}

impl Skifio {
    fn new() -> (Self, SkifioHandle) {
        let (dac_send, dac_recv) = channel();
        let (adcs_send, adcs_recv) = channel();
        let (dout_send, dout_recv) = channel();
        let (din_send, din_recv) = channel();
        let last_din = Arc::new(AtomicDin::default());
        let din_handler = Arc::new(Mutex::new(None::<Box<dyn skifio::DinHandler>>));
        {
            let mut recv = din_recv;
            let handler = din_handler.clone();
            let last = last_din.clone();
            spawn(async move {
                loop {
                    let din = recv.next().await.unwrap();
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
                adcs: adcs_recv,
                last_adcs: None,
                dout: dout_send,
                last_din,
                din_handler,
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
        Ok(())
    }
    fn dac_state(&self) -> bool {
        true
    }

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        if self.last_adcs.is_some() {
            return Ok(());
        }
        let fut = async {
            match timeout {
                Some(to) => select_biased! {
                    xs = self.adcs.next().fuse() => Some(xs.unwrap()),
                    () = sleep(to).fuse() => None,
                },
                None => Some(self.adcs.next().await.unwrap()),
            }
        };
        match block_on(fut) {
            Some(xs) => {
                self.last_adcs.replace(xs);
                Ok(())
            }
            None => Err(Error {
                kind: ErrorKind::TimedOut,
                source: ErrorSource::None,
            }),
        }
    }
    fn transfer(&mut self, out: skifio::XferOut) -> Result<skifio::XferIn, Error> {
        if self.last_adcs.is_none() {
            self.last_adcs
                .replace(self.adcs.try_next().unwrap().unwrap());
        }
        self.dac.unbounded_send(out.dac).unwrap();
        Ok(skifio::XferIn {
            adcs: self.last_adcs.take().unwrap(),
        })
    }

    fn write_dout(&mut self, dout: skifio::Dout) -> Result<(), Error> {
        self.dout.unbounded_send(dout).unwrap();
        Ok(())
    }

    fn read_din(&mut self) -> skifio::Din {
        self.last_din.load(std::sync::atomic::Ordering::Acquire)
    }
    fn subscribe_din(
        &mut self,
        callback: Option<Box<dyn skifio::DinHandler>>,
    ) -> Result<(), Error> {
        *self.din_handler.lock().unwrap() = callback;
        Ok(())
    }
}

pub fn bind() -> SkifioHandle {
    let (skifio, handle) = Skifio::new();
    assert!(SKIFIO.lock().unwrap().replace(Box::new(skifio)).is_none());
    handle
}
