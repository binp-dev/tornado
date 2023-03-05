use async_std::{
    main as async_main,
    task::{sleep, spawn},
};
use common::config::{dac_to_volt, volt_to_adc, Point, ADC_COUNT, SAMPLE_PERIOD};
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
        loop {
            let mut adcs = [0; ADC_COUNT];
            let dac = dac_to_volt(skifio.dac.next().await.unwrap() as Point);
            adcs[0] = volt_to_adc(dac);
            for i in 1..ADC_COUNT {
                adcs[i] = volt_to_adc(phases[i].sin());
                phases[i] = 2.0 * PI * FREQS[i] * counter as f64 * SAMPLE_PERIOD.as_secs_f64();
            }
            skifio.adcs.send(adcs).await.unwrap();
            unsafe { user_sample_intr() };

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
