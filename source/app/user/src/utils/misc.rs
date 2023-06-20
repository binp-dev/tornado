// TODO: Remove when `array::unzip` stabilized.
pub fn unzip_array<T, U, const N: usize>(arr: [(T, U); N]) -> ([T; N], [U; N]) {
    let (mut aa, mut bb) = ([0; N].map(|_| None), [0; N].map(|_| None));
    for (i, (a, b)) in arr.into_iter().enumerate() {
        aa[i] = Some(a);
        bb[i] = Some(b);
    }
    (aa.map(|x| x.unwrap()), bb.map(|x| x.unwrap()))
}
