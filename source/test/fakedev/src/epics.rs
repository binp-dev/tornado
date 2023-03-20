use common::config::ADC_COUNT;
use cstr::cstr;
use epics_ca::{types::EpicsEnum, Context, ValueChannel as Channel};
use futures::{stream::iter, StreamExt};
use std::{ffi::CString, future::Future};

pub struct Dac {
    pub array: Channel<[f64]>,
    pub scalar: Channel<f64>,
    pub request: Channel<EpicsEnum>,
    pub state: Channel<EpicsEnum>,
    pub mode: Channel<EpicsEnum>,
}

pub struct Adc {
    pub array: Channel<[f64]>,
    pub scalar: Channel<f64>,
}

pub struct Epics {
    pub dac: Dac,
    pub adc: [Adc; ADC_COUNT],
    pub dout: Channel<i32>,
    pub din: Channel<i32>,
}

async fn make_array<T, G: Future<Output = T>, F: Fn(usize) -> G, const N: usize>(f: F) -> [T; N] {
    let vec: Vec<_> = iter(0..N).then(f).collect().await;
    assert_eq!(vec.len(), N);
    vec.try_into().ok().unwrap()
}

impl Epics {
    pub async fn connect(ctx: &Context) -> Self {
        Self {
            dac: Dac {
                array: ctx.connect(cstr!("aao0")).await.unwrap(),
                scalar: ctx.connect(cstr!("ao0")).await.unwrap(),
                request: ctx.connect(cstr!("aao0_request")).await.unwrap(),
                state: ctx.connect(cstr!("aao0_state")).await.unwrap(),
                mode: ctx.connect(cstr!("aao0_mode")).await.unwrap(),
            },
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
            dout: ctx.connect(cstr!("do0")).await.unwrap(),
            din: ctx.connect(cstr!("di0")).await.unwrap(),
        }
    }
}
