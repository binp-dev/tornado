#![no_std]
#![allow(clippy::missing_safety_doc)]

pub mod error;
pub use error::Error;

#[cfg(feature = "real")]
mod hal;

pub mod buffers;
pub mod channel;
pub mod skifio;
pub mod tasks;

extern crate alloc;

use ringbuf::traits::SplitRef;
use ustd::{println, task::Priority};

const CONTROL_TASK_PRIORITY: Priority = 4 as Priority;
const RPMSG_WRITE_TASK_PRIORITY: Priority = 3 as Priority;
const RPMSG_READ_TASK_PRIORITY: Priority = 2 as Priority;

#[no_mangle]
pub extern "C" fn user_main() {
    println!("Enter user code");

    let dac_buffer = buffers::DAC_BUFFER.take().unwrap();
    let adc_buffer = buffers::ADC_BUFFER.take().unwrap();
    let (dac_producer, dac_consumer) = dac_buffer.split_ref();
    let (adc_producer, adc_consumer) = adc_buffer.split_ref();
    let stats = tasks::STATISTICS.clone();

    let (control, handle) = tasks::Control::new(dac_consumer, adc_producer, stats.clone());
    let rpmsg = tasks::Rpmsg::new(handle, dac_producer, adc_consumer, stats.clone());

    println!("Starting tasks ...");
    control.run(CONTROL_TASK_PRIORITY);
    rpmsg.run(RPMSG_READ_TASK_PRIORITY, RPMSG_WRITE_TASK_PRIORITY);
    #[cfg(feature = "real")]
    stats.run_printer(core::time::Duration::from_secs(10));
}
