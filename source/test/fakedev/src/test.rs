mod testing;

use ca::types::EpicsEnum;
use epics_ca as ca;
use fakedev::{run, Epics};
use futures::{join, FutureExt};
use indicatif::{MultiProgress, ProgressBar, ProgressStyle};
use mcu::tasks::STATISTICS;
use testing::{adc, dac, dio};
use tokio::{main as async_main, task::spawn};

#[async_main]
async fn main() {
    const ATTEMPTS: usize = 64;
    const CYCLIC_ATTEMPTS: usize = 16;
    const PREFIX: &str = "tornado0:";

    let m = MultiProgress::new();
    let sty = ProgressStyle::with_template("{prefix:18} [{wide_bar}] {pos:>4}/{len:4}")
        .unwrap()
        .progress_chars("=> ");

    let skifio = run();
    let ctx = ca::Context::new().unwrap();
    let epics = Epics::connect(&ctx, PREFIX).await;
    let (dac_m, dac_sty) = (m.clone(), sty.clone());
    let dac = spawn(async move {
        let context = dac::Context {
            epics: epics.dac,
            device: skifio.dac,
        };

        let (m, sty) = (dac_m, dac_sty);
        let ppb = m.add(
            ProgressBar::new(ATTEMPTS as u64)
                .with_style(sty.clone())
                .with_prefix("DAC.IOC"),
        );
        let cpb = m.insert_after(
            &ppb,
            ProgressBar::new(ATTEMPTS as u64)
                .with_style(sty.clone())
                .with_prefix("DAC.SkifIO"),
        );
        let mut context = dac::test(context, ATTEMPTS, (ppb, cpb.clone())).await;

        let ppb = m.insert_after(
            &cpb,
            ProgressBar::new(1)
                .with_style(sty.clone())
                .with_prefix("DAC(Cyclic).IOC"),
        );
        let cpb = m.insert_after(
            &ppb,
            ProgressBar::new(CYCLIC_ATTEMPTS as u64)
                .with_style(sty.clone())
                .with_prefix("DAC(Cyclic).SkifIO"),
        );
        context.epics.mode.put(EpicsEnum(1)).unwrap().await.unwrap();
        dac::test_cyclic(context, CYCLIC_ATTEMPTS, (ppb, cpb)).await;
    })
    .map(Result::unwrap);
    let adc = spawn({
        let attempts = ATTEMPTS + CYCLIC_ATTEMPTS;
        let ppb = m.add(
            ProgressBar::new(attempts as u64)
                .with_style(sty.clone())
                .with_prefix("ADC.SkifIO"),
        );
        let cpb = m.insert_after(
            &ppb,
            ProgressBar::new(attempts as u64)
                .with_style(sty.clone())
                .with_prefix("ADC.IOC"),
        );
        adc::test(epics.adc, skifio.adcs, attempts, (ppb, cpb))
    })
    .map(Result::unwrap);
    let dout = spawn(dio::test_dout(
        epics.dout,
        skifio.dout,
        ATTEMPTS,
        m.add(
            ProgressBar::new(ATTEMPTS as u64)
                .with_style(sty.clone())
                .with_prefix("Dout"),
        ),
    ))
    .map(Result::unwrap);
    let din = spawn(dio::test_din(
        epics.din,
        skifio.din,
        ATTEMPTS,
        m.add(
            ProgressBar::new(ATTEMPTS as u64)
                .with_style(sty.clone())
                .with_prefix("Din"),
        ),
    ))
    .map(Result::unwrap);
    join!(dac, adc, dout, din);

    println!("Statistics: {}", STATISTICS.as_ref());
}
