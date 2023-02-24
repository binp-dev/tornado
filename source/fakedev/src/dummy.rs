use async_std::{
    main as async_main,
    stream::StreamExt,
    task::{sleep, spawn},
};
use common::config::{dac_to_volt, volt_to_adc, Point, ADC_COUNT, SAMPLE_PERIOD};
use fakedev::run;
use std::{f64::consts::PI, time::Duration};

const FREQS: [f64; ADC_COUNT] = [0.0, 1.0, PI, 10.0, 10.0 * PI, 100.0];

#[async_main]
async fn main() {
    let mut skifio = run();
    let mut phases = [0.0_f64; ADC_COUNT];
    spawn(async move {
        let mut counter = 0;
        loop {
            let mut adcs = [0; ADC_COUNT];
            let dac = dac_to_volt(skifio.dac.next().await.unwrap() as Point);
            adcs[0] = volt_to_adc(dac);
            for i in 1..ADC_COUNT {
                adcs[i] = volt_to_adc(phases[i].sin());
                phases[i] += 2.0 * PI * FREQS[i] * SAMPLE_PERIOD.as_secs_f64();
            }
            skifio.adcs.unbounded_send(adcs).unwrap();

            const BATCH: usize = 100;
            counter += 1;
            if counter % BATCH == 0 {
                sleep(SAMPLE_PERIOD * BATCH as u32).await;
            }
        }
    });
    spawn(async move {
        loop {
            skifio
                .din
                .unbounded_send(skifio.dout.next().await.unwrap())
                .unwrap();
        }
    });
    loop {
        sleep(Duration::from_millis(100)).await;
    }
}
