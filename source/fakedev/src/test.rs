use approx::assert_abs_diff_eq;
use async_std::{
    main as async_main,
    task::{sleep, spawn},
};
use common::config::{
    dac_to_volt, volt_to_adc, ADC_COUNT, ADC_MAX_ABS, ADC_STEP, DAC_MAX_ABS, DAC_STEP,
};
use epics_ca::{types::EpicsEnum, Context};
use fakedev::{epics, run, Epics};
use futures::{
    channel::mpsc::{Receiver, Sender},
    future::join_all,
    join, pin_mut, SinkExt, StreamExt,
};
use mcu::{
    skifio::{Ain, Aout},
    tasks::STATISTICS,
};
use std::{
    f64::consts::PI,
    io::{stdout, Write},
    iter::repeat_with,
    time::Duration,
};

extern "C" {
    fn user_sample_intr();
}

const ATTEMPTS: usize = 64;

async fn test_dac(mut epics: epics::Dac, mut device: Receiver<Aout>) {
    let len = epics.array.element_count().unwrap();
    let data = (0..ATTEMPTS).into_iter().map(move |j| {
        (0..len)
            .into_iter()
            .map(move |i| i as f64 / (len - 1) as f64)
            .map(move |x| (2.0 * PI * (j + 1) as f64 * x).sin() * DAC_MAX_ABS)
    });

    let prod = spawn({
        let mut data = data.clone();
        async move {
            let request = epics.request.subscribe();
            pin_mut!(request);
            loop {
                let flag = request.next().await.unwrap().unwrap();
                //println!("@@ request.next: {:?}", flag);
                if flag == EpicsEnum(0) {
                    continue;
                }
                let wf = match data.next() {
                    Some(iter) => iter.collect::<Vec<_>>(),
                    None => break,
                };
                epics.array.put_ref(&wf).unwrap().await.unwrap();
                print!("O");
                stdout().flush().unwrap();
            }
            println!("@@ dac prod done");
        }
    });

    let cons = spawn(async move {
        let mut seq = data.flatten();
        for _ in 0..(ATTEMPTS * len) {
            let dac = device.next().await.unwrap();
            assert_abs_diff_eq!(
                dac_to_volt(dac as i32),
                seq.next().unwrap(),
                epsilon = DAC_STEP
            );
        }
        println!("@@ dac cons done");
    });

    join!(prod, cons);
}

async fn test_adc(mut epics: [epics::Adc; ADC_COUNT], mut device: Sender<[Ain; ADC_COUNT]>) {
    sleep(Duration::from_millis(1000)).await;

    let len = epics
        .iter()
        .map(|adc| adc.array.element_count().unwrap())
        .fold(None, |a, x| {
            if let Some(y) = a {
                assert_eq!(x, y);
            }
            Some(x)
        })
        .unwrap();
    let total_len = len * ATTEMPTS;
    let mut data = (0..total_len)
        .into_iter()
        .map(move |i| i as f64 / (len - 1) as f64)
        .map(move |x| {
            {
                let mut k = 0;
                [(); ADC_COUNT].map(|()| {
                    let r = k;
                    k += 1;
                    r
                })
            }
            .map(move |k| (2.0 * PI * (k + 1) as f64 * x).sin() * ADC_MAX_ABS * x)
        });

    let prod = spawn({
        let data = data.clone();
        async move {
            for xs in data {
                let adcs = xs.map(volt_to_adc);
                device.send(adcs).await.unwrap();
                unsafe { user_sample_intr() };
            }
            println!("@@ adc prod done");
        }
    });

    let cons = spawn(async move {
        let mut arrays = epics
            .iter_mut()
            .map(|adc| Box::pin(adc.array.subscribe_vec()))
            .collect::<Vec<_>>();
        let mut count = 0;
        loop {
            let wfs = join_all(
                arrays
                    .iter_mut()
                    .map(|sub| async { sub.next().await.unwrap().unwrap() }),
            )
            .await;
            let points = {
                let mut iters = wfs.into_iter().map(|wf| wf.into_iter()).collect::<Vec<_>>();
                repeat_with(move || {
                    let mut res = [None; ADC_COUNT];
                    for (i, a) in iters.iter_mut().enumerate() {
                        res[i] = a.next();
                    }
                    res[0]?;
                    Some(res.map(|x| x.unwrap()))
                })
                .take_while(|x| x.is_some())
                .map(|x| x.unwrap())
                .collect::<Vec<_>>()
            };
            count += points.len();
            for xs in points.into_iter() {
                xs.into_iter()
                    .zip(data.next().unwrap())
                    .for_each(|(x, y)| assert_abs_diff_eq!(x, y, epsilon = ADC_STEP));
            }
            print!("I");
            stdout().flush().unwrap();
            if count == total_len {
                break;
            }
            assert!(count < total_len);
        }
        println!("@@ adc cons done");
    });

    join!(prod, cons);
}

#[async_main]
async fn main() {
    let skifio = run();
    let ctx = Context::new().unwrap();
    let epics = Epics::connect(&ctx).await;
    let dac = {
        let [dac] = epics.dac;
        spawn(test_dac(dac, skifio.dac))
    };
    let adc = spawn(test_adc(epics.adc, skifio.adcs));
    join!(dac, adc);

    println!("Statistics: {}", STATISTICS.as_ref());
}
