use super::scale;
use approx::assert_abs_diff_eq;
use common::values::Point;
use epics_ca::types::EpicsEnum;
use fakedev::epics;
use futures::{join, pin_mut, FutureExt, StreamExt};
use indicatif::ProgressBar;
use std::f64::consts::PI;
use tokio::{sync::mpsc::Receiver, task::spawn};

pub struct Context {
    pub epics: epics::Dac,
    pub device: Receiver<Point>,
}

pub async fn test(
    mut context: Context,
    attempts: usize,
    pbs: (ProgressBar, ProgressBar),
) -> Context {
    let len = context.epics.array.element_count().unwrap();
    let data = (0..attempts).map(move |j| {
        (0..len)
            .map(move |i| i as f64 / (len - 1) as f64)
            .map(move |x| scale((2.0 * PI * (j + 1) as f64 * x).sin()))
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
                    pbs.0.inc(1);
                }
                pbs.0.finish();
            }
            epics
        }
    })
    .map(Result::unwrap);

    let cons = spawn(async move {
        let mut seq = data.flatten();
        for i in 0..(attempts * len) {
            let dac = context.device.recv().await.unwrap();
            assert_abs_diff_eq!(
                dac.into_analog(),
                seq.next().unwrap(),
                epsilon = Point::STEP
            );
            if (i + 1) % len == 0 {
                pbs.1.inc(1);
            }
        }
        pbs.1.finish();
        context.device
    })
    .map(Result::unwrap);

    let (epics, device) = join!(prod, cons);

    Context { epics, device }
}

pub async fn test_cyclic(mut context: Context, attempts: usize, pbs: (ProgressBar, ProgressBar)) {
    let len = context.epics.array.element_count().unwrap();
    let data = (0..len)
        .map(move |i| i as f64 / (len - 1) as f64)
        .map(move |x| x * scale((2.0 * PI * x).sin()));

    let prod = spawn({
        let data = data.clone().collect::<Vec<_>>();
        let mut epics = context.epics;
        async move {
            let request = epics.request.subscribe();
            pin_mut!(request);
            while request.next().await.unwrap().unwrap() == EpicsEnum(0) {}
            epics.array.put_ref(&data).unwrap().await.unwrap();
            pbs.0.inc(1);
            pbs.0.finish();
        }
    })
    .map(Result::unwrap);

    let cons = spawn(async move {
        let mut seq = data.into_iter().cycle().take(len * attempts);
        for i in 0..(attempts * len) {
            let dac = context.device.recv().await.unwrap();
            assert_abs_diff_eq!(
                dac.into_analog(),
                seq.next().unwrap(),
                epsilon = Point::STEP
            );
            if (i + 1) % len == 0 {
                pbs.1.inc(1);
            }
        }
        pbs.1.finish();
    })
    .map(Result::unwrap);

    join!(prod, cons);
}
