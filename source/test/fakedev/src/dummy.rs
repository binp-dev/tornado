use async_std::{
    main as async_main,
    task::{sleep, spawn},
};
use common::{
    config::{ADC_COUNT, SAMPLE_PERIOD},
    units::{AdcPoint, Unit, Voltage},
};
use fakedev::run;
use futures::{SinkExt, StreamExt};
use std::{f64::consts::PI, time::Duration};

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
        let mut adcs = [AdcPoint::ZERO; ADC_COUNT];
        loop {
            skifio.adcs.send(adcs).await.unwrap();
            unsafe { user_sample_intr() };

            let dac = skifio.dac.next().await.unwrap().to_voltage();
            adcs[0] = AdcPoint::try_from_voltage(dac).unwrap();
            for i in 1..ADC_COUNT {
                adcs[i] = AdcPoint::try_from_voltage(phases[i].sin()).unwrap();
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
                .send(skifio.dout.next().await.unwrap())
                .await
                .unwrap();
        }
    });
    loop {
        sleep(Duration::from_millis(100)).await;
    }
}
