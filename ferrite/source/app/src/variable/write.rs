use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    mem::drop,
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
        WriteFuture {
            owner: self,
            value,
            complete: false,
        }
    }
}

pub struct WriteFuture<'a, T: Copy> {
    owner: &'a mut WriteVariable<T>,
    value: T,
    complete: bool,
}

impl<'a, T: Copy> Unpin for WriteFuture<'a, T> {}

impl<'a, T: Copy> Future for WriteFuture<'a, T> {
    type Output = ();

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        assert!(!self.complete);
        let val = self.value;
        let mut guard = self.owner.raw.lock_mut();
        let ps = guard.proc_state();
        if !ps.processing {
            if !ps.requested {
                ps.set_waker(cx.waker());
                unsafe { guard.request_proc() };
            }
            return Poll::Pending;
        }
        unsafe { *(guard.data_mut_ptr() as *mut T) = val };
        unsafe { guard.complete_proc() };
        drop(guard);
        self.complete = true;
        Poll::Ready(())
    }
}

impl<'a, T: Copy> Drop for WriteFuture<'a, T> {
    fn drop(&mut self) {
        let guard = self.owner.raw.lock();
        if !self.complete {
            guard.proc_state().clean_waker();
        }
    }
}
