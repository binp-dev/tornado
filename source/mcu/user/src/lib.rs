#![no_std]

mod error;
pub use error::Error;

#[cfg(feature = "real")]
mod real;
#[cfg(feature = "real")]
pub use real::*;

#[cfg(feature = "emul")]
mod emul;
#[cfg(feature = "emul")]
pub use emul::*;

pub mod buffers;
pub mod skifio;
pub mod tasks;

extern crate alloc;

use core::time::Duration;
use ustd::prelude::*;

const CONTROL_TASK_PRIORITY: usize = 4;
const RPMSG_WRITE_TASK_PRIORITY: usize = 3;
const RPMSG_READ_TASK_PRIORITY: usize = 2;

#[no_mangle]
pub extern "C" fn user_main() {
    println!("Enter user code");

    let dac_buffer = &buffers::DAC_BUFFER;
    let adc_buffer = &buffers::ADC_BUFFER;
    let (dac_producer, dac_consumer) =
        unsafe { (buffers::DacProducer::new(dac_buffer), buffers::DacConsumer::new(dac_buffer)) };
    let (adc_producer, adc_consumer) =
        unsafe { (buffers::AdcProducer::new(adc_buffer), buffers::AdcConsumer::new(adc_buffer)) };
    let stats = tasks::STATISTICS.clone();

    let (control, handle) = tasks::Control::new(dac_consumer, adc_producer, stats.clone());
    let rpmsg = tasks::Rpmsg::new(handle, dac_producer, adc_consumer, dac_buffer, stats.clone());

    println!("Running tasks...");
    control.run(CONTROL_TASK_PRIORITY);
    rpmsg.run(RPMSG_READ_TASK_PRIORITY, RPMSG_WRITE_TASK_PRIORITY);
    stats.run_printer(Duration::from_secs(10));

    println!("Done");
}
