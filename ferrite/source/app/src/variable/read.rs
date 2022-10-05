use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    pin::Pin,
    task::{Context, Poll},
};

pub struct ReadVariable<T: Copy> {
    raw: raw::Variable,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadVariable<T> {
    pub(crate) fn from_raw(raw: raw::Variable) -> Self {
        Self {
            raw,
            _phantom: PhantomData,
        }
    }

    pub fn read(&mut self) -> ReadFuture<'_, T> {
        ReadFuture {
            owner: self,
            complete: false,
        }
    }
}

pub struct ReadFuture<'a, T: Copy> {
    owner: &'a mut ReadVariable<T>,
    complete: bool,
}

impl<'a, T: Copy> Unpin for ReadFuture<'a, T> {}

impl<'a, T: Copy> Future for ReadFuture<'a, T> {
    type Output = T;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<T> {
        assert!(!self.complete);
        let ps = self.owner.raw.proc_state();
        ps.set_waker(cx.waker());
        if !ps.processing() {
            if !ps.requested() {
                unsafe { self.owner.raw.lock().request_proc() };
            }
            Poll::Pending
        } else {
            let val;
            {
                let mut guard = self.owner.raw.lock();
                val = unsafe { *(guard.data_ptr() as *const T) };
                unsafe { guard.complete_proc() };
            }
            self.complete = true;
            Poll::Ready(val)
        }
    }
}

impl<'a, T: Copy> Drop for ReadFuture<'a, T> {
    fn drop(&mut self) {
        if !self.complete {
            self.owner.raw.proc_state().clean_waker();
        }
    }
}
