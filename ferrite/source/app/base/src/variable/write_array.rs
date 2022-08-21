use crate::raw;
use std::marker::PhantomData;

pub struct WriteArrayVariable<T: Copy> {
    raw: raw::VarLock,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> WriteArrayVariable<T> {
    pub(crate) unsafe fn from_raw(raw: raw::Var, max_len: usize) -> Self {
        Self {
            raw: raw::VarLock::new(raw),
            max_len,
            _phantom: PhantomData,
        }
    }
}
