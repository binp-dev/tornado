use crate::channel::{rpmsg::Writer as RpmsgWriter, Rpmsg};
use common::values::{volt_to_uv_saturating, Uv};
use flatty_io::Writer;
use lazy_static::lazy_static;
use std::{mem::size_of, sync::Mutex};

lazy_static! {
    pub static ref WRITER: Mutex<Option<FastWriter>> = Mutex::new(None);
}

#[no_mangle]
pub extern "C" fn app_set_dac_corr(value: f64) {
    WRITER
        .lock()
        .unwrap()
        .as_mut()
        .unwrap()
        .write_dac_add(volt_to_uv_saturating(value));
}

pub struct FastWriter {
    writer: Writer<Uv, RpmsgWriter>,
}

impl FastWriter {
    pub async fn new() -> Self {
        let channel = Rpmsg::open("/dev/ttyRPMSG1").await.unwrap();
        let (_, w) = channel.split_blocking();
        Self {
            writer: Writer::<Uv, _>::new(w, size_of::<Uv>()),
        }
    }

    pub fn write_dac_add(&mut self, value: Uv) {
        self.writer
            .alloc_message()
            .new_in_place(value)
            .unwrap()
            .write()
            .unwrap();
    }
}

pub async fn init_global() {
    assert!(WRITER
        .lock()
        .unwrap()
        .replace(FastWriter::new().await)
        .is_none());
}
