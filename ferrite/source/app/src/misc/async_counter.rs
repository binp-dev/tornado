use core::{
    future::Future,
    ops::{Bound, RangeBounds},
    pin::Pin,
    sync::atomic::{AtomicUsize, Ordering},
    task::{Context, Poll},
};
use futures::task::AtomicWaker;

pub struct AsyncCounter {
    value: AtomicUsize,
    waker: AtomicWaker,
}

impl AsyncCounter {
    pub fn new(value: usize) -> Self {
        Self {
            value: AtomicUsize::new(value),
            waker: AtomicWaker::new(),
        }
    }
    pub fn add(&self, value: usize) {
        self.value.fetch_add(value, Ordering::SeqCst);
        self.waker.wake();
    }
    pub fn wait_sub<R: RangeBounds<usize>>(&self, range: R) -> WaitSubFuture<'_, R> {
        WaitSubFuture { owner: self, range }
    }
}

pub struct WaitSubFuture<'a, R: RangeBounds<usize>> {
    owner: &'a AsyncCounter,
    range: R,
}
impl<'a, R: RangeBounds<usize>> Unpin for WaitSubFuture<'a, R> {}
impl<'a, R: RangeBounds<usize>> Future for WaitSubFuture<'a, R> {
    type Output = usize;

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<usize> {
        self.owner.waker.register(cx.waker());
        let start = match self.range.start_bound() {
            Bound::Included(x) => *x,
            Bound::Excluded(x) => {
                assert_ne!(*x, usize::MAX);
                *x + 1
            }
            Bound::Unbounded => 0,
        };
        let value = self.owner.value.load(Ordering::Acquire);
        if value < start {
            return Poll::Pending;
        }
        let sub_value = match self.range.end_bound() {
            Bound::Included(x) => usize::min(*x, value),
            Bound::Excluded(x) => {
                assert_ne!(*x, 0);
                usize::min(*x - 1, value)
            }
            Bound::Unbounded => value,
        };
        self.owner.value.fetch_sub(sub_value, Ordering::SeqCst);
        Poll::Ready(sub_value)
    }
}
