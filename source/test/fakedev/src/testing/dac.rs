use super::scale;
use approx::assert_abs_diff_eq;
use async_std::task::spawn;
use common::units::{DacPoint, Voltage};
use epics_ca::types::EpicsEnum;
use fakedev::epics;
use futures::{channel::mpsc::Receiver, join, pin_mut, StreamExt};
use std::{
    f64::consts::PI,
    io::{stdout, Write},
};

pub struct Context {
    pub epics: epics::Dac,
    pub device: Receiver<DacPoint>,
}

pub async fn test(mut context: Context, attempts: usize) -> Context {
    let len = context.epics.array.element_count().unwrap();
    let data = (0..attempts).map(move |j| {
        (0..len)
            .map(move |i| i as f64 / (len - 1) as f64)
            .map(move |x| scale::<DacPoint>((2.0 * PI * (j + 1) as f64 * x).sin()))
    });

    let prod = spawn({
        let mut data = data.clone();
        let mut epics = context.epics;
        async move {
            {
                let request = epics.request.subscribe();
                pin_mut!(request);
                loop {
                    let flag = request.next().await.unwrap().unwrap();
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
            epics
        }
    });

    let cons = spawn(async move {
        let mut seq = data.flatten();
        for _ in 0..(attempts * len) {
            let dac = context.device.next().await.unwrap();
            assert_abs_diff_eq!(
                dac.to_voltage(),
                seq.next().unwrap(),
                epsilon = DacPoint::STEP
            );
        }
        println!("@@ dac cons done");
        context.device
    });

    let (epics, device) = join!(prod, cons);

    Context { epics, device }
}

pub async fn test_cyclic(mut context: Context, attempts: usize) {
    let len = context.epics.array.element_count().unwrap();
    let data = (0..len)
        .map(move |i| i as f64 / (len - 1) as f64)
        .map(move |x| x * scale::<DacPoint>((2.0 * PI * x).sin()));

    let prod = spawn({
        let data = data.clone().collect::<Vec<_>>().clone();
        let mut epics = context.epics;
        async move {
            let request = epics.request.subscribe();
            pin_mut!(request);
            while request.next().await.unwrap().unwrap() == EpicsEnum(0) {}
            epics.array.put_ref(&data).unwrap().await.unwrap();
            print!("C");
            stdout().flush().unwrap();
            println!("@@ dac cyclic prod done");
        }
    });

    let cons = spawn(async move {
        let mut seq = data.into_iter().cycle().take(len * attempts);
        for _ in 0..(attempts * len) {
            let dac = context.device.next().await.unwrap();
            assert_abs_diff_eq!(
                dac.to_voltage(),
                seq.next().unwrap(),
                epsilon = DacPoint::STEP
            );
        }
        println!("@@ dac cyclic cons done");
    });

    join!(prod, cons);
}
