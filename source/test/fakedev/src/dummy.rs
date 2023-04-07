use common::{
    config::{ADC_COUNT, SAMPLE_PERIOD},
    values::Point,
};
use fakedev::run;
use std::{f64::consts::PI, time::Duration};
use tokio::{main as async_main, task::spawn, time::sleep};

const FREQS: [f64; ADC_COUNT] = [0.0, 1.0, PI, 10.0, 10.0 * PI, 100.0];

extern "C" {
    fn user_sample_intr();
}

#[async_main]
async fn main() {
    let mut skifio = run();
    let mut phases = [0.0_f64; ADC_COUNT];
    spawn(async move {
        let mut counter: u64 = 0;
        let mut adcs = [Point::default(); ADC_COUNT];
        loop {
            skifio.adcs.send(adcs).await.unwrap();
            unsafe { user_sample_intr() };

            let dac = skifio.dac.recv().await.unwrap();
            adcs[0] = Point::try_from_analog(dac.into_analog()).unwrap();
            for i in 1..ADC_COUNT {
                adcs[i] = Point::try_from_analog(phases[i].sin()).unwrap();
                phases[i] = 2.0 * PI * FREQS[i] * counter as f64 * SAMPLE_PERIOD.as_secs_f64();
            }

            const BATCH: usize = 1000;
            counter += 1;
            if counter % BATCH as u64 == 0 {
                sleep(SAMPLE_PERIOD * BATCH as u32).await;
            }
        }
    });
    spawn(async move {
        loop {
            skifio
                .din
                .send(
                    u8::from(skifio.dout.recv().await.unwrap())
                        .try_into()
                        .unwrap(),
                )
                .await
                .unwrap();
        }
    });
    loop {
        sleep(Duration::from_millis(100)).await;
    }
}
