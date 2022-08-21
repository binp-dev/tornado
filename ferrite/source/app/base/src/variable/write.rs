use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    pin::Pin,
    task::{Context, Poll},
};

pub struct WriteVariable<T: Copy> {
    raw: raw::Variable,
    _phantom: PhantomData<T>,
}

impl<T: Copy> WriteVariable<T> {
    pub(crate) fn from_raw(raw: raw::Variable) -> Self {
        Self {
            raw,
            _phantom: PhantomData,
        }
    }

    pub fn write(&mut self, value: T) -> WriteFuture<'_, T> {
        WriteFuture { owner: self, value }
    }
}

pub struct WriteFuture<'a, T: Copy> {
    owner: &'a mut WriteVariable<T>,
    value: T,
}

impl<'a, T: Copy> Future for WriteFuture<'a, T> {
    type Output = ();

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        let val = self.value;
        let mut guard = self.owner.raw.lock_mut();
        let ps = guard.proc_state();
        ps.set_waker(cx.waker());
        if !ps.processing {
            unsafe { guard.request_proc() };
            return Poll::Pending;
        }
        unsafe { *(guard.data_mut_ptr() as *mut T) = val };
        unsafe { guard.complete_proc() };
        Poll::Ready(())
    }
}

impl<'a, T: Copy> Unpin for WriteFuture<'a, T> {}
