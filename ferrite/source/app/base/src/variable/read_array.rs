use crate::raw;
use std::marker::PhantomData;

//pub struct ReadArrayGuard<T: Copy> {}

pub struct ReadArrayVariable<T: Copy> {
    raw: raw::VarLock,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadArrayVariable<T> {
    pub(crate) unsafe fn from_raw(raw: raw::Var, max_len: usize) -> Self {
        Self {
            raw: raw::VarLock::new(raw),
            max_len,
            _phantom: PhantomData,
        }
    }
}
