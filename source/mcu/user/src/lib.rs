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
pub mod wf;

extern crate alloc;

use ringbuf::traits::SplitRef;
use ustd::{println, task::Priority};

const CONTROL_TASK_PRIORITY: Priority = 4 as Priority;
const RPMSG_WRITE_TASK_PRIORITY: Priority = 3 as Priority;
const RPMSG_READ_TASK_PRIORITY: Priority = 2 as Priority;

#[no_mangle]
pub extern "C" fn user_main() {
    println!("Enter user code");

    let ao_buffer = buffers::AO_BUFFER.take().unwrap();
    let ai_buffer = buffers::AI_BUFFER.take().unwrap();
    let (ao_producer, ao_consumer) = ao_buffer.split_ref();
    let (ai_producer, ai_consumer) = ai_buffer.split_ref();
    let stats = tasks::STATISTICS.clone();

    let (control, handle) = tasks::Control::new(ao_consumer, ai_producer, stats.clone());
    let rpmsg = tasks::Rpmsg::new(handle, ao_producer, ai_consumer, stats.clone());

    println!("Starting tasks ...");
    control.run(CONTROL_TASK_PRIORITY);
    rpmsg.run(RPMSG_READ_TASK_PRIORITY, RPMSG_WRITE_TASK_PRIORITY);
    #[cfg(feature = "real")]
    stats.run_printer(core::time::Duration::from_secs(10));
}
