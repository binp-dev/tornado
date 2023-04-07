use common::config::{ADC_COUNT, DIN_BITS, DOUT_BITS};
use epics_ca::{
    error,
    types::{EpicsEnum, Value},
    Context, Error, ValueChannel as Channel,
};
use futures::{stream::iter, StreamExt};
use std::{
    ffi::{CStr, CString},
    future::Future,
    time::Duration,
};
use tokio::time::timeout;

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
    pub dout: [Channel<u8>; DOUT_BITS],
    pub din: [Channel<u8>; DIN_BITS],
}

async fn make_array<T, G: Future<Output = T>, F: Fn(usize) -> G, const N: usize>(f: F) -> [T; N] {
    let vec: Vec<_> = iter(0..N).then(f).collect().await;
    assert_eq!(vec.len(), N);
    vec.try_into().ok().unwrap()
}

macro_rules! cformat {
    ($( $arg:tt )*) => {
        CString::new(format!($( $arg )*)).unwrap()
    };
}

async fn connect<V>(ctx: &Context, name: &CStr) -> Result<Channel<V>, Error>
where
    V: Value + ?Sized,
{
    const TIMEOUT: Duration = Duration::from_secs(1);
    match timeout(TIMEOUT, ctx.connect(name)).await {
        Ok(res) => res,
        Err(_) => Err(error::TIMEOUT),
    }
}

impl Epics {
    pub async fn connect(ctx: &Context, prefix: &str) -> Self {
        Self {
            dac: Dac {
                array: connect(ctx, &cformat!("{}aao0", prefix)).await.unwrap(),
                scalar: connect(ctx, &cformat!("{}ao0", prefix)).await.unwrap(),
                request: connect(ctx, &cformat!("{}aao0_request", prefix))
                    .await
                    .unwrap(),
                state: connect(ctx, &cformat!("{}aao0_state", prefix))
                    .await
                    .unwrap(),
                mode: connect(ctx, &cformat!("{}aao0_mode", prefix))
                    .await
                    .unwrap(),
            },
            adc: make_array(|i| async move {
                Adc {
                    array: connect(ctx, &cformat!("{}aai{}", prefix, i)).await.unwrap(),
                    scalar: connect(ctx, &cformat!("{}ai{}", prefix, i)).await.unwrap(),
                }
            })
            .await,
            dout: async {
                let nobt = connect::<i16>(ctx, &cformat!("{}do0.NOBT", prefix))
                    .await
                    .unwrap()
                    .get()
                    .await
                    .unwrap();
                assert_eq!(nobt as usize, DOUT_BITS);

                make_array(|i| async move {
                    connect(ctx, &cformat!("{}do0.B{:X}", prefix, i))
                        .await
                        .unwrap()
                })
                .await
            }
            .await,
            din: async {
                let nobt = connect::<i16>(ctx, &cformat!("{}di0.NOBT", prefix))
                    .await
                    .unwrap()
                    .get()
                    .await
                    .unwrap();
                assert_eq!(nobt as usize, DIN_BITS);

                make_array(|i| async move {
                    connect(ctx, &cformat!("{}di0.B{:X}", prefix, i))
                        .await
                        .unwrap()
                })
                .await
            }
            .await,
        }
    }
}
