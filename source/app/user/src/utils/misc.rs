use core::future::Future;
use ferrite::{typed::Type, TypedVariable as Variable};
use futures::{
    future::Map,
    stream::{self, Stream},
    FutureExt,
};
use tokio::task::{self, JoinError, JoinHandle};

// TODO: Remove when `array::unzip` stabilized.
pub fn unzip_array<T, U, const N: usize>(arr: [(T, U); N]) -> ([T; N], [U; N]) {
    let (mut aa, mut bb) = ([0; N].map(|_| None), [0; N].map(|_| None));
    for (i, (a, b)) in arr.into_iter().enumerate() {
        aa[i] = Some(a);
        bb[i] = Some(b);
    }
    (aa.map(|x| x.unwrap()), bb.map(|x| x.unwrap()))
}

pub fn unfold_variable<T: Send, V: Type, F: Fn(V) -> Option<T>>(
    var: Variable<V>,
    map: F,
) -> impl Stream<Item = T> {
    stream::unfold((var, map), move |(mut var, map)| async move {
        let x = loop {
            if let Some(x) = map(var.wait().await.read().await) {
                break x;
            }
        };
        Some((x, (var, map)))
    })
}

fn unwrap<T>(r: Result<T, JoinError>) -> T {
    r.unwrap()
}

pub fn spawn<T: Send + 'static, F: Future<Output = T> + Send + 'static>(
    future: F,
) -> Map<JoinHandle<T>, fn(Result<T, JoinError>) -> T> {
    task::spawn(future).map(unwrap)
}
