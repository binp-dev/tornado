use core::{
    future::Future,
    pin::Pin,
    sync::atomic::{AtomicBool, Ordering},
    task::{Context, Poll},
};
use futures::task::AtomicWaker;

pub struct AsyncFlag {
    value: AtomicBool,
    waker: AtomicWaker,
}

impl AsyncFlag {
    pub fn new(value: bool) -> Self {
        Self {
            value: AtomicBool::new(value),
            waker: AtomicWaker::new(),
        }
    }

    pub fn set(&self) {
        self.value.store(true, Ordering::Release);
        self.waker.wake();
    }

    pub fn get(&self) -> bool {
        self.value.load(Ordering::Acquire)
    }
    pub fn take(&self) -> bool {
        self.value.fetch_and(false, Ordering::SeqCst)
    }

    pub fn wait(&self) -> WaitFuture<'_> {
        WaitFuture {
            owner: self,
            take: false,
        }
    }
    pub fn wait_take(&self) -> WaitFuture<'_> {
        WaitFuture { owner: self, take: true }
    }
}

pub struct WaitFuture<'a> {
    owner: &'a AsyncFlag,
    take: bool,
}
impl<'a> Unpin for WaitFuture<'a> {}
impl<'a> Future for WaitFuture<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        self.owner.waker.register(cx.waker());
        let value = if self.take { self.owner.take() } else { self.owner.get() };
        if value {
            Poll::Ready(())
        } else {
            Poll::Pending
        }
    }
}
