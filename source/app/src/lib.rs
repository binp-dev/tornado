mod channel;
mod config;
mod device;
mod epics;
mod proto;

use async_std::net::TcpStream;
use ferrite::{entry_point, Context};
use futures::executor::block_on;
use macro_rules_attribute::apply;

use device::Device;
use epics::Epics;

/// *Export symbols being called from IOC.*
pub use ferrite::export;

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    block_on(async_main(ctx));
}

async fn async_main(ctx: Context) {
    println!("[app]: Start IOC");

    println!("[app]: Establish channel");
    let channel = TcpStream::connect("127.0.0.1:4884").await.unwrap();

    println!("[app]: Get EPICS PVs");
    let epics = Epics::new(ctx).unwrap();

    println!("[app]: Run device");
    let device = Device::new(channel, epics);
    device.run().await;

    unreachable!();
}
