use std::{
    future::Future,
    marker::PhantomData,
    pin::Pin,
    task::{Context, Poll},
};

use crate::raw::{self, variable::ProcState};

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
        let value = self.value;
        let info = self.owner.raw.info();
        info.set_waker(cx.waker());
        match info.proc_state() {
            ProcState::Idle => unsafe { self.owner.raw.lock().request_proc() },
            ProcState::Requested => (),
            ProcState::Processing => {
                let mut guard = self.owner.raw.lock();
                unsafe { *(guard.data_mut_ptr() as *mut T) = value };
                unsafe { guard.complete_proc() };
            }
            ProcState::Ready => (),
            ProcState::Complete => {
                unsafe { self.owner.raw.lock().clean_proc() };
                self.complete = true;
                return Poll::Ready(());
            }
        }
        Poll::Pending
    }
}
