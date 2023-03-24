mod testing;

use async_std::{main as async_main, task::spawn};
use ca::types::EpicsEnum;
use epics_ca as ca;
use fakedev::{run, Epics};
use futures::join;
use mcu::tasks::STATISTICS;
use testing::{adc, dac};

#[async_main]
async fn main() {
    const ATTEMPTS: usize = 64;
    const CYCLIC_ATTEMPTS: usize = 16;

    let skifio = run();
    let ctx = ca::Context::new().unwrap();
    let epics = Epics::connect(&ctx).await;
    let dac = spawn(async {
        let context = dac::Context {
            epics: epics.dac,
            device: skifio.dac,
        };
        let mut context = dac::test(context, ATTEMPTS).await;
        context.epics.mode.put(EpicsEnum(1)).unwrap().await.unwrap();
        dac::test_cyclic(context, CYCLIC_ATTEMPTS).await;
    });
    let adc = spawn(adc::test(
        epics.adc,
        skifio.adcs,
        ATTEMPTS + CYCLIC_ATTEMPTS,
    ));
    join!(dac, adc);

    println!("Statistics: {}", STATISTICS.as_ref());
}
