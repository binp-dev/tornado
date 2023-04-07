use ferrite::{typed::Type, TypedVariable as Variable};
use futures::stream::{self, Stream};

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
