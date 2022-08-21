use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    pin::Pin,
    task::{Context, Poll},
};

pub struct WriteVariable<T: Copy> {
    raw: raw::VarLock,
    _phantom: PhantomData<T>,
}

impl<T: Copy> WriteVariable<T> {
    pub(crate) unsafe fn from_raw(raw: raw::Var) -> Self {
        Self {
            raw: raw::VarLock::new(raw),
            _phantom: PhantomData,
        }
    }

    pub fn write(&mut self, val: T) -> WriteFuture<'_, T> {
        WriteFuture { var: self, val }
    }
}

pub struct WriteFuture<'a, T: Copy> {
    var: &'a mut WriteVariable<T>,
    val: T,
}

impl<'a, T: Copy> Future for WriteFuture<'a, T> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        let mut guard = unsafe { self.var.raw.lock() };
        let ps = unsafe { guard.proc_state() };
        ps.set_waker(cx.waker());
        if !ps.processing {
            unsafe { guard.req_proc() };
            return Poll::Pending;
        }
        unsafe { *(guard.data_mut() as *mut T) = self.val };
        unsafe { guard.proc_done() };
        Poll::Ready(())
    }
}
