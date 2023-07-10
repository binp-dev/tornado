use common::config::{AI_COUNT, DI_BITS, DO_BITS};
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

pub struct Ao {
    pub waveform: Channel<[f64]>,
    pub ready: Channel<EpicsEnum>,
    pub cyclic: Channel<EpicsEnum>,
}

pub struct Ai {
    pub waveform: Channel<[f64]>,
}

pub struct Epics {
    pub ao: Ao,
    pub ais: [Ai; AI_COUNT],
    pub do_: [Channel<u8>; DO_BITS],
    pub di: [Channel<u8>; DI_BITS],
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
            ao: Ao {
                waveform: connect(ctx, &cformat!("{}Ao0Next", prefix)).await.unwrap(),
                ready: connect(ctx, &cformat!("{}AoNextReady", prefix))
                    .await
                    .unwrap(),
                cyclic: connect(ctx, &cformat!("{}AoNextCycle", prefix))
                    .await
                    .unwrap(),
            },
            ais: make_array(|i| async move {
                Ai {
                    waveform: connect(ctx, &cformat!("{}Ai{}", prefix, i)).await.unwrap(),
                }
            })
            .await,
            do_: async {
                let nobt = connect::<i16>(ctx, &cformat!("{}Do.NOBT", prefix))
                    .await
                    .unwrap()
                    .get()
                    .await
                    .unwrap();
                assert_eq!(nobt as usize, DO_BITS);

                make_array(|i| async move {
                    connect(ctx, &cformat!("{}Do.B{:X}", prefix, i))
                        .await
                        .unwrap()
                })
                .await
            }
            .await,
            di: async {
                let nobt = connect::<i16>(ctx, &cformat!("{}Di.NOBT", prefix))
                    .await
                    .unwrap()
                    .get()
                    .await
                    .unwrap();
                assert_eq!(nobt as usize, DI_BITS);

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
