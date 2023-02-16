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

pub mod tasks;

extern crate alloc;

use crate::{
    channel::{Channel, ReadChannel, Reader, WriteChannel, Writer},
    hal::RetCode,
};
use alloc::sync::Arc;
use common::protocol::{self as proto, AppMsg, McuMsg};
use core::time::Duration;
use freertos::{Duration as FreeRtosDuration, Semaphore, Task, TaskDelay};

fn rpmsg_init_task(this: Task) {
    println!("RPMsg init task started");
    let (reader, writer) = Channel::new(&this, 0).unwrap().split();
    let semaphore = Arc::new(Semaphore::new_binary().unwrap());

    {
        let semaphore = semaphore.clone();
        Task::new()
            .start(move |task| rpmsg_recv_task(task, reader, semaphore))
            .unwrap();
    }
    Task::new()
        .start(move |task| rpmsg_send_task(task, writer, semaphore))
        .unwrap();
    println!("RPMsg init task complete");
}

fn rpmsg_recv_task(_: Task, channel: ReadChannel, semaphore: Arc<Semaphore>) {
    println!("RPMsg recv task started");
    let mut reader = Reader::<AppMsg>::new(channel, Some(Duration::from_secs(1)));

    loop {
        let msg = match reader.read_message() {
            Ok(msg) => msg,
            Err(Error::Hal(RetCode::TimedOut)) => {
                println!("Read message timed out");
                continue;
            }
            Err(other) => panic!("Read message error: {:?}", other),
        };

        use proto::AppMsgRef;
        print!("Message read: ");
        match msg.as_ref() {
            AppMsgRef::Connect => {
                println!("Connect");
                semaphore.give();
            }
            AppMsgRef::KeepAlive => println!("KeepAlive"),
            AppMsgRef::DacMode { enable } => println!("DacMode {{ enable: {} }}", enable),
            AppMsgRef::DacData { points } => {
                println!("DacMode {{ points.len(): {} }}", points.len())
            }
            AppMsgRef::DoutUpdate { value } => println!("DoutUpdate {{ value: 0b{:b} }}", value),
            AppMsgRef::StatsReset => println!("StatsReset"),
        }
    }
}

fn rpmsg_send_task(_: Task, channel: WriteChannel, semaphore: Arc<Semaphore>) {
    use flatty::{portable::le, vec::FromIterator};

    println!("RPMsg send task started");

    let mut writer = Writer::<McuMsg>::new(channel, None);

    semaphore.take(FreeRtosDuration::infinite()).unwrap();
    writer
        .new_message()
        .unwrap()
        .emplace(proto::McuMsgInitDebug {
            message: FromIterator("Hello from MCU".as_bytes().iter().cloned()),
        })
        .unwrap()
        .write()
        .unwrap();
    println!("Message sent: Hello");

    loop {
        TaskDelay::new().delay_until(FreeRtosDuration::ms(1000));
        writer
            .new_message()
            .unwrap()
            .emplace(proto::McuMsgInitDacRequest {
                count: le::U32::from(200),
            })
            .unwrap()
            .write()
            .unwrap();
        println!("Message sent: Points request");
    }
}

#[no_mangle]
pub extern "C" fn user_main() {
    println!("Starting RPMsg task...\n");
    Task::new().start(rpmsg_init_task).unwrap();
}

/*
lazy_static!{
    static ref DAC_BUFFER: StaticRb<DacPoint, DAC_BUFFER_LEN> = StaticRb::new();
    static ref ADC_BUFFER: StaticRb<AdcPoint, ADC_BUFFER_LEN> = StaticRb::new();
}
*/
