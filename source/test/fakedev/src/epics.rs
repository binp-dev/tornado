use common::config::{AI_COUNT, DIN_BITS, DOUT_BITS};
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
    pub request: Channel<EpicsEnum>,
    pub mode: Channel<EpicsEnum>,
}

pub struct Adc {
    pub array: Channel<[f64]>,
}

pub struct Epics {
    pub dac: Dac,
    pub adc: [Adc; AI_COUNT],
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
                array: connect(ctx, &cformat!("{}Ao0Next", prefix)).await.unwrap(),
                request: connect(ctx, &cformat!("{}AoNextReady", prefix))
                    .await
                    .unwrap(),
                mode: connect(ctx, &cformat!("{}AoNextCycle", prefix))
                    .await
                    .unwrap(),
            },
            adc: make_array(|i| async move {
                Adc {
                    array: connect(ctx, &cformat!("{}Ai{}", prefix, i)).await.unwrap(),
                }
            })
            .await,
            dout: async {
                let nobt = connect::<i16>(ctx, &cformat!("{}Do.NOBT", prefix))
                    .await
                    .unwrap()
                    .get()
                    .await
                    .unwrap();
                assert_eq!(nobt as usize, DOUT_BITS);

                make_array(|i| async move {
                    connect(ctx, &cformat!("{}Do.B{:X}", prefix, i))
                        .await
                        .unwrap()
                })
                .await
            }
            .await,
            din: async {
                let nobt = connect::<i16>(ctx, &cformat!("{}Di.NOBT", prefix))
                    .await
                    .unwrap()
                    .get()
                    .await
                    .unwrap();
                assert_eq!(nobt as usize, DIN_BITS);

                make_array(|i| async move {
                    connect(ctx, &cformat!("{}Di.B{:X}", prefix, i))
                        .await
                        .unwrap()
                })
                .await
            }
            .await,
        }
    }
}
