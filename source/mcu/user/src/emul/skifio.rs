use crate::Error;
use core::time::Duration;

use crate::skifio::{Din, DinHandler, Dout, XferIn, XferOut};

pub fn handle() -> Option<Skifio> {
    unimplemented!()
}

pub struct Skifio {
    _unused: [u8; 0],
}

impl Skifio {
    fn new() -> Self {
        Self { _unused: [] }
    }

    pub fn set_dac_state(&mut self, enabled: bool) -> Result<(), Error> {
        unimplemented!()
    }
    pub fn dac_state(&self) -> bool {
        unimplemented!()
    }

    pub fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        unimplemented!()
    }
    pub fn transfer(&mut self, out: XferOut) -> Result<XferIn, Error> {
        unimplemented!()
    }

    pub fn write_dout(&mut self, dout: Dout) -> Result<(), Error> {
        unimplemented!()
    }

    pub fn read_din(&mut self) -> Din {
        unimplemented!()
    }
    pub fn subscribe_din<F: DinHandler + Send + 'static>(&mut self, callback: Option<F>) -> Result<(), Error> {
        unimplemented!()
    }
}
