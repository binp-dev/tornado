mod channel;
mod config;
mod device;
mod epics;
mod proto;

use ferrite::{entry_point, Context};
use futures::{
    executor::{block_on, ThreadPool},
    future::pending,
};
use macro_rules_attribute::apply;
use std::sync::Arc;

use device::Device;
use epics::Epics;

/// *Export symbols being called from IOC.*
pub use ferrite::export;

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    env_logger::init();
    let exec = Arc::new(ThreadPool::builder().pool_size(2).create().unwrap());
    exec.spawn_ok(run(exec.clone(), ctx));
    // TODO: Wait for exec to complete all tasks.
    block_on(pending::<()>());
}

async fn run(exec: Arc<ThreadPool>, ctx: Context) {
    log::info!("Start IOC");

    log::info!("Establish channel");
    #[cfg(feature = "tcp")]
    let channel = channel::TcpStream::connect("127.0.0.1:4884").await.unwrap();
    #[cfg(feature = "rpmsg")]
    let channel = channel::Rpmsg::open("/dev/ttyRPMSG0").await.unwrap();

    log::info!("Get EPICS PVs");
    let epics = Epics::new(ctx).unwrap();

    log::info!("Run device");
    let device = Device::new(channel, epics);
    device.run(exec).await;
}
