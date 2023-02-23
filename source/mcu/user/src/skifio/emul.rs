extern crate std;

use super::SkifioIface;
use lazy_static::lazy_static;
use std::{boxed::Box, sync::Mutex};

pub type Skifio = Box<dyn SkifioIface>;

lazy_static! {
    pub static ref SKIFIO: Mutex<Option<Skifio>> = Mutex::new(None);
}

pub fn handle() -> Option<Skifio> {
    SKIFIO.lock().unwrap().take()
}
