use crate::raw;
use std::{
    future::Future,
    marker::PhantomData,
    mem::{drop, MaybeUninit},
    ops::Deref,
    pin::Pin,
    slice,
    task::{Context, Poll},
};

pub struct WriteArrayVariable<T: Copy> {
    raw: raw::Variable,
    max_len: usize,
    _phantom: PhantomData<T>,
}

impl<T: Copy> WriteArrayVariable<T> {
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

    pub fn write_in_place(&mut self) -> WriteInPlaceFuture<'_, T> {
        WriteInPlaceFuture { owner: Some(self) }
    }

    pub async fn write_from_slice(&mut self, src: &[T]) {
        assert!(src.len() <= self.max_len);
        let mut guard = self.write_in_place().await;
        let dst_uninit = guard.as_uninit_slice();
        let src_uninit =
            unsafe { slice::from_raw_parts(src.as_ptr() as *const MaybeUninit<T>, src.len()) };
        dst_uninit[..src.len()].copy_from_slice(&src_uninit);
        guard.set_len(src.len());
    }
}

pub struct WriteInPlaceFuture<'a, T: Copy> {
    owner: Option<&'a mut WriteArrayVariable<T>>,
}

impl<'a, T: Copy> Future for WriteInPlaceFuture<'a, T> {
    type Output = WriteArrayGuard<'a, T>;

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
        Poll::Ready(WriteArrayGuard::new(owner))
    }
}

impl<'a, T: Copy> Unpin for WriteInPlaceFuture<'a, T> {}

pub struct WriteArrayGuard<'a, T: Copy> {
    owner: &'a mut WriteArrayVariable<T>,
}

impl<'a, T: Copy> WriteArrayGuard<'a, T> {
    fn new(owner: &'a mut WriteArrayVariable<T>) -> Self {
        unsafe { owner.raw.get_unprotected().lock() };
        Self { owner }
    }

    pub fn as_uninit_slice(&mut self) -> &mut [MaybeUninit<T>] {
        let max_len = self.owner.max_len;
        unsafe {
            let raw_unprotected = self.owner.raw.get_unprotected();
            std::slice::from_raw_parts_mut(
                *(raw_unprotected.data_ptr() as *const *mut MaybeUninit<T>),
                max_len,
            )
        }
    }

    pub fn set_len(&mut self, new_len: usize) {
        assert!(new_len <= self.owner.max_len);
        unsafe { self.owner.raw.get_unprotected_mut() }.array_set_len(new_len);
    }
}

impl<'a, T: Copy> Drop for WriteArrayGuard<'a, T> {
    fn drop(&mut self) {
        unsafe {
            let raw_unprotected = self.owner.raw.get_unprotected_mut();
            raw_unprotected.complete_proc();
            raw_unprotected.unlock();
        }
    }
}
