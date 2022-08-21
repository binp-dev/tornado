pub mod any;
pub mod registry;
pub mod type_;

pub use any::AnyVariable;
pub use type_::VariableType;

use crate::sys;
use std::marker::PhantomData;

pub struct ReadVariable<T: Copy> {
    raw: sys::VarLock,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadVariable<T> {
    unsafe fn from_raw(raw: sys::Var) -> Self {
        Self {
            raw: sys::VarLock::new(raw),
            _phantom: PhantomData,
        }
    }

    pub fn read_current(&mut self) -> T {
        unsafe {
            let guard = self.raw.lock();
            *(guard.data() as *mut T)
        }
    }

    // pub async fn read_next() -> T {}
}

pub struct WriteVariable<T: Copy> {
    raw: sys::VarLock,
    _phantom: PhantomData<T>,
}

impl<T: Copy> WriteVariable<T> {
    unsafe fn from_raw(raw: sys::Var) -> Self {
        Self {
            raw: sys::VarLock::new(raw),
            _phantom: PhantomData,
        }
    }

    // async fn write() -> T {}
}

//pub struct ReadArrayGuard<T: Copy> {}

pub struct ReadArrayVariable<T: Copy> {
    raw: sys::VarLock,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadArrayVariable<T> {
    unsafe fn from_raw(raw: sys::Var, max_len: usize) -> Self {
        Self {
            raw: sys::VarLock::new(raw),
            max_len,
            _phantom: PhantomData,
        }
    }
}

pub struct WriteArrayVariable<T: Copy> {
    raw: sys::VarLock,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> WriteArrayVariable<T> {
    unsafe fn from_raw(raw: sys::Var, max_len: usize) -> Self {
        Self {
            raw: sys::VarLock::new(raw),
            max_len,
            _phantom: PhantomData,
        }
    }
}
