use super::{scale, user_sample_intr};
use approx::assert_abs_diff_eq;
use common::{
    config::ADC_COUNT,
    values::{try_volt_to_uv, Uv, VOLT_EPS},
};
use fakedev::epics;
use futures::{future::join_all, join, FutureExt, StreamExt};
use indicatif::ProgressBar;
use std::{
    f64::consts::PI,
    io::{stdout, Write},
    iter::repeat_with,
    time::Duration,
};
use tokio::{sync::mpsc::Sender, task::spawn, time::sleep};

pub async fn test(
    mut epics: [epics::Adc; ADC_COUNT],
    device: Sender<[Uv; ADC_COUNT]>,
    attempts: usize,
    pbs: (ProgressBar, ProgressBar),
) {
    sleep(Duration::from_millis(100)).await;

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
    let total_len = len * attempts;
    let mut data = (0..total_len)
        .map(move |i| i as f64 / (total_len - 1) as f64)
        .map(move |x| {
            {
                let mut k = 0;
                [(); ADC_COUNT].map(|()| {
                    let r = k;
                    k += 1;
                    r
                })
            }
            .map(move |k| scale((2.0 * PI * (k + 1) as f64 * x * attempts as f64).sin()) * x)
        });

    let prod = spawn({
        let data = data.clone();
        async move {
            for (i, xs) in data.enumerate() {
                let adcs = xs.map(|x| try_volt_to_uv(x).unwrap());
                device.send(adcs).await.unwrap();
                unsafe { user_sample_intr() };
                if (i + 1) % len == 0 {
                    pbs.0.inc(1);
                }
            }
            pbs.0.finish();
        }
    })
    .map(Result::unwrap);

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
                    .for_each(|(x, y)| assert_abs_diff_eq!(x, y, epsilon = VOLT_EPS));
            }
            pbs.1.inc(1);
            stdout().flush().unwrap();
            if count == total_len {
                break;
            }
            assert!(count < total_len);
        }
        pbs.1.finish();
    })
    .map(Result::unwrap);

    join!(prod, cons);
}
