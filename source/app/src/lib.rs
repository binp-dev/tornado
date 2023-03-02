mod channel;
mod device;
mod epics;

use async_std::task::block_on;
#[cfg(feature = "tcp")]
use common::config;
use ferrite::{entry_point, Context};
use macro_rules_attribute::apply;

use device::Device;
use epics::Epics;

/// *Export symbols being called from IOC.*
pub use ferrite::export;

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();
    block_on(run(ctx));
}

async fn run(ctx: Context) {
    log::info!("Start IOC");

    log::info!("Establish channel");
    #[cfg(feature = "tcp")]
    let channel = channel::connect(config::CHANNEL_ADDR).await.unwrap();
    #[cfg(feature = "rpmsg")]
    let channel = channel::Rpmsg::open("/dev/ttyRPMSG0").await.unwrap();
    log::info!("Connection established");

    log::info!("Get EPICS PVs");
    let epics = Epics::new(ctx).unwrap();

    log::info!("Run device");
    let device = Device::new(channel, epics);
    device.run(exec).await;
}
