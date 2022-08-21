use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    mem,
    ops::Deref,
    pin::Pin,
    task::{Context, Poll},
};

pub struct ReadArrayVariable<T: Copy> {
    raw: raw::Variable,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadArrayVariable<T> {
    pub(crate) fn from_raw(raw: raw::Variable, max_len: usize) -> Self {
        Self {
            raw,
            max_len,
            _phantom: PhantomData,
        }
    }
}
