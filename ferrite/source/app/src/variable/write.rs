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
        let ps = self.owner.raw.proc_state();
        ps.set_waker(cx.waker());
        if !ps.processing() {
            if !ps.requested() {
                unsafe { self.owner.raw.lock().request_proc() };
            }
            Poll::Pending
        } else {
            {
                let mut guard = self.owner.raw.lock();
                unsafe { *(guard.data_mut_ptr() as *mut T) = val };
                unsafe { guard.complete_proc() };
            }
            self.complete = true;
            Poll::Ready(())
        }
    }
}

impl<'a, T: Copy> Drop for WriteFuture<'a, T> {
    fn drop(&mut self) {
        if !self.complete {
            self.owner.raw.proc_state().clean_waker();
        }
    }
}
