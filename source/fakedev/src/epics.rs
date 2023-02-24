use common::config::{ADC_COUNT, DAC_COUNT};
use epics_ca::{Context, ValueChannel as Channel};
use futures::{stream::iter, StreamExt};
use std::{ffi::CString, future::Future};

pub struct Dac {
    pub array: Channel<[f64]>,
    pub scalar: Channel<f64>,
    pub request: Channel<i32>,
    pub state: Channel<i32>,
    pub mode: Channel<i32>,
}

pub struct Adc {
    pub array: Channel<[f64]>,
    pub scalar: Channel<f64>,
}

pub struct Epics {
    pub dac: [Dac; DAC_COUNT],
    pub adc: [Adc; ADC_COUNT],
    pub dout: Channel<f64>,
    pub din: Channel<f64>,
}

async fn make_array<T, G: Future<Output = T>, F: Fn(usize) -> G, const N: usize>(f: F) -> [T; N] {
    let vec: Vec<_> = iter(0..DAC_COUNT).then(f).collect().await;
    assert_eq!(vec.len(), N);
    vec.try_into().ok().unwrap()
}

impl Epics {
    pub async fn connect(ctx: &Context) -> Self {
        Self {
            dac: make_array(|i| async move {
                Dac {
                    array: ctx
                        .connect(&CString::new(format!("aao{}", i)).unwrap())
                        .await
                        .unwrap(),
                    scalar: ctx
                        .connect(&CString::new(format!("ao{}", i)).unwrap())
                        .await
                        .unwrap(),
                    request: ctx
                        .connect(&CString::new(format!("aao{}_request", i)).unwrap())
                        .await
                        .unwrap(),
                    state: ctx
                        .connect(&CString::new(format!("aao{}_state", i)).unwrap())
                        .await
                        .unwrap(),
                    mode: ctx
                        .connect(&CString::new(format!("aao{}_mode", i)).unwrap())
                        .await
                        .unwrap(),
                }
            })
            .await,
            adc: make_array(|i| async move {
                Adc {
                    array: ctx
                        .connect(&CString::new(format!("aai{}", i)).unwrap())
                        .await
                        .unwrap(),
                    scalar: ctx
                        .connect(&CString::new(format!("ai{}", i)).unwrap())
                        .await
                        .unwrap(),
                }
            })
            .await,
            dout: ctx.connect(&CString::new("do0").unwrap()).await.unwrap(),
            din: ctx.connect(&CString::new("di0").unwrap()).await.unwrap(),
        }
    }
}
