mod channel;
mod device;
mod epics;
mod utils;

#[cfg(feature = "tcp")]
use common::config;
use ferrite::{entry_point, Context};
use macro_rules_attribute::apply;
use tokio::runtime;

use device::Device;
use epics::Epics;

pub use device::export as device_export;
/// *Export symbols being called from IOC.*
pub use ferrite::export;

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    let rt = runtime::Builder::new_multi_thread()
        .enable_all()
        .build()
        .unwrap();
    let _guard = rt.enter();

    rt.block_on(run(ctx));
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

    log::info!("Init device");
    let device = Device::new(channel, epics).await;
    log::info!("Run device");
    device.run().await;

    log::info!("device stopped");
}
