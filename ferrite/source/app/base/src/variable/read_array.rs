use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    mem::drop,
    ops::Deref,
    pin::Pin,
    task::{Context, Poll},
};

pub struct ReadArrayVariable<T: Copy> {
    raw: raw::Variable,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> ReadArrayVariable<T> {
    pub(crate) fn from_raw(raw: raw::Variable, max_len: usize) -> Self {
        Self {
            raw,
            max_len,
            _phantom: PhantomData,
        }
    }

    pub fn max_len(&self) -> usize {
        self.max_len
    }

    pub fn read_in_place(&mut self) -> ReadInPlaceFuture<'_, T> {
        ReadInPlaceFuture { owner: Some(self) }
    }

    pub async fn read_to_slice(&mut self, dst: &mut [T]) -> Option<usize> {
        let src = self.read_in_place().await;
        if dst.len() >= src.len() {
            dst[..src.len()].copy_from_slice(&src);
            Some(src.len())
        } else {
            None
        }
    }
}

pub struct ReadInPlaceFuture<'a, T: Copy> {
    owner: Option<&'a mut ReadArrayVariable<T>>,
}

impl<'a, T: Copy> Future for ReadInPlaceFuture<'a, T> {
    type Output = ReadArrayGuard<'a, T>;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let owner = self.owner.take().unwrap();
        let mut guard = owner.raw.lock_mut();
        let ps = guard.proc_state();
        ps.set_waker(cx.waker());
        if !ps.processing {
            unsafe { guard.request_proc() };
            drop(guard);
            self.owner.replace(owner);
            return Poll::Pending;
        }
        drop(guard);
        Poll::Ready(ReadArrayGuard::new(owner))
    }
}

impl<'a, T: Copy> Unpin for ReadInPlaceFuture<'a, T> {}

pub struct ReadArrayGuard<'a, T: Copy> {
    owner: &'a mut ReadArrayVariable<T>,
}

impl<'a, T: Copy> ReadArrayGuard<'a, T> {
    fn new(owner: &'a mut ReadArrayVariable<T>) -> Self {
        unsafe { owner.raw.get_unprotected().lock() };
        Self { owner }
    }

    pub fn as_slice(&self) -> &[T] {
        unsafe {
            let raw_unprotected = self.owner.raw.get_unprotected();
            std::slice::from_raw_parts(
                *(raw_unprotected.data_ptr() as *const *const T),
                raw_unprotected.array_len(),
            )
        }
    }
}

impl<'a, T: Copy> Deref for ReadArrayGuard<'a, T> {
    type Target = [T];
    fn deref(&self) -> &[T] {
        self.as_slice()
    }
}

impl<'a, T: Copy> Drop for ReadArrayGuard<'a, T> {
    fn drop(&mut self) {
        unsafe {
            let raw_unprotected = self.owner.raw.get_unprotected_mut();
            raw_unprotected.complete_proc();
            raw_unprotected.unlock();
        }
    }
}
