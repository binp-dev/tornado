use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    pin::Pin,
    task::{Context, Poll},
};

pub struct ReadVariable<T: Copy> {
    raw: raw::VarLock,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadVariable<T> {
    pub(crate) unsafe fn from_raw(raw: raw::Var) -> Self {
        Self {
            raw: raw::VarLock::new(raw),
            _phantom: PhantomData,
        }
    }

    pub fn read(&mut self) -> ReadFuture<'_, T> {
        ReadFuture { var: self }
    }
}

pub struct ReadFuture<'a, T: Copy> {
    var: &'a mut ReadVariable<T>,
}

impl<'a, T: Copy> Future for ReadFuture<'a, T> {
    type Output = T;

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<T> {
        let mut guard = unsafe { self.var.raw.lock() };
        let ps = unsafe { guard.proc_state() };
        ps.set_waker(cx.waker());
        if !ps.processing {
            unsafe { guard.req_proc() };
            return Poll::Pending;
        }
        let val = unsafe { *(guard.data() as *const T) };
        unsafe { guard.proc_done() };
        Poll::Ready(val)
    }
}
