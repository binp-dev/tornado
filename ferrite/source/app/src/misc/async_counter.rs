use core::{
    future::Future,
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
    pub fn sub(&self, max_value: Option<usize>) -> usize {
        let mut value = self.value.load(Ordering::Acquire);
        if let Some(x) = max_value {
            value = usize::min(value, x);
        }
        self.value.fetch_sub(value, Ordering::SeqCst);
        value
    }
    pub fn wait(&self, min_value: usize) -> WaitFuture<'_> {
        WaitFuture { owner: self, min_value }
    }
}

pub struct WaitFuture<'a> {
    owner: &'a AsyncCounter,
    min_value: usize,
}
impl<'a> Unpin for WaitFuture<'a> {}
impl<'a> Future for WaitFuture<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        self.owner.waker.register(cx.waker());
        let value = self.owner.value.load(Ordering::Acquire);
        if value < self.min_value {
            return Poll::Pending;
        }
        Poll::Ready(())
    }
}
