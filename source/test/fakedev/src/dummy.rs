use common::{
    config::{AI_COUNT, DO_BITS, SAMPLE_PERIOD},
    values::{try_volt_to_uv, Uv},
};
use fakedev::run;
use std::{f64::consts::PI, time::Duration};
use tokio::{main as async_main, task::spawn, time::sleep};

const FREQS: [f64; AI_COUNT] = [0.0, 1.0, PI, 10.0, 10.0 * PI, 100.0];

extern "C" {
    fn user_sample_intr();
}

#[async_main]
async fn main() {
    let mut skifio = run();
    let mut phases = [0.0_f64; AI_COUNT];
    spawn(async move {
        let mut counter: u64 = 0;
        let mut ais = [Uv::default(); AI_COUNT];
        loop {
            skifio.ais.send(ais).await.unwrap();
            unsafe { user_sample_intr() };

            let ao = skifio.ao.recv().await.unwrap();
            ais[0] = ao;
            for i in 1..AI_COUNT {
                ais[i] = try_volt_to_uv(phases[i].sin()).unwrap();
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
            let mut value = u8::from(skifio.do_.recv().await.unwrap());
            value |= value << DO_BITS;
            skifio.di.send(value.try_into().unwrap()).await.unwrap();
        }
    });
    loop {
        sleep(Duration::from_millis(100)).await;
    }
}
