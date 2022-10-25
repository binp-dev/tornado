use std::{
    future::Future,
    marker::PhantomData,
    mem::MaybeUninit,
    pin::Pin,
    slice,
    task::{Context, Poll},
};

use crate::raw::{self, variable::ProcState};

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

    pub fn init_in_place(&mut self) -> InitInPlaceFuture<'_, T> {
        InitInPlaceFuture { owner: Some(self) }
    }

    pub async fn write_from_slice(&mut self, src: &[T]) {
        assert!(src.len() <= self.max_len);
        let mut guard = self.init_in_place().await;
        let dst_uninit = guard.as_uninit_slice();
        let src_uninit = unsafe { slice::from_raw_parts(src.as_ptr() as *const MaybeUninit<T>, src.len()) };
        dst_uninit[..src.len()].copy_from_slice(src_uninit);
        guard.set_len(src.len());
    }
}

pub struct InitInPlaceFuture<'a, T: Copy> {
    owner: Option<&'a mut WriteArrayVariable<T>>,
}

impl<'a, T: Copy> Unpin for InitInPlaceFuture<'a, T> {}

impl<'a, T: Copy> Future for InitInPlaceFuture<'a, T> {
    type Output = WriteArrayGuard<'a, T>;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let owner = self.owner.take().unwrap();
        let info = owner.raw.info();
        info.set_waker(cx.waker());
        match info.proc_state() {
            ProcState::Idle => unsafe { owner.raw.lock().request_proc() },
            ProcState::Requested => (),
            ProcState::Processing => return Poll::Ready(WriteArrayGuard::new(owner)),
            _ => unreachable!(),
        }
        self.owner.replace(owner);
        Poll::Pending
    }
}

#[must_use]
pub struct WriteArrayGuard<'a, T: Copy> {
    owner: Option<&'a mut WriteArrayVariable<T>>,
}

impl<'a, T: Copy> WriteArrayGuard<'a, T> {
    fn new(owner: &'a mut WriteArrayVariable<T>) -> Self {
        unsafe { owner.raw.get_unprotected().lock() };
        Self { owner: Some(owner) }
    }

    pub fn as_uninit_slice(&mut self) -> &mut [MaybeUninit<T>] {
        let owner = self.owner.as_ref().unwrap();
        let max_len = owner.max_len;
        unsafe {
            let raw_unprotected = owner.raw.get_unprotected();
            std::slice::from_raw_parts_mut(raw_unprotected.data_ptr() as *mut MaybeUninit<T>, max_len)
        }
    }

    pub fn set_len(&mut self, new_len: usize) {
        let owner = self.owner.as_mut().unwrap();
        assert!(new_len <= owner.max_len);
        unsafe { owner.raw.get_unprotected_mut() }.array_set_len(new_len);
    }

    pub fn write(mut self) -> WriteArrayFuture<'a, T> {
        let owner = self.owner.take().unwrap();
        unsafe {
            let raw_unprotected = owner.raw.get_unprotected_mut();
            raw_unprotected.complete_proc();
            raw_unprotected.unlock();
        }
        WriteArrayFuture { owner: Some(owner) }
    }
}

impl<'a, T: Copy> Drop for WriteArrayGuard<'a, T> {
    fn drop(&mut self) {
        if let Some(_owner) = self.owner.take() {
            panic!("WriteArrayGuard must be explicitly written");
        }
    }
}

pub struct WriteArrayFuture<'a, T: Copy> {
    owner: Option<&'a mut WriteArrayVariable<T>>,
}

impl<'a, T: Copy> Unpin for WriteArrayFuture<'a, T> {}

impl<'a, T: Copy> Future for WriteArrayFuture<'a, T> {
    type Output = ();

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let owner = self.owner.take().unwrap();
        let info = owner.raw.info();
        info.set_waker(cx.waker());
        match info.proc_state() {
            ProcState::Ready => (),
            ProcState::Complete => {
                unsafe { owner.raw.clean_proc() };
                return Poll::Ready(());
            }
            _ => unreachable!(),
        }
        self.owner.replace(owner);
        Poll::Pending
    }
}
