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

    pub fn value(&self) -> bool {
        self.value.load(Ordering::Acquire)
    }

    pub fn try_give(&self) -> bool {
        if !self.value.fetch_or(true, Ordering::SeqCst) {
            self.waker.wake();
            true
        } else {
            false
        }
    }
    pub fn try_take(&self) -> bool {
        if self.value.fetch_and(false, Ordering::SeqCst) {
            self.waker.wake();
            true
        } else {
            false
        }
    }

    pub fn wait(&self, value: bool) -> Wait<'_> {
        Wait {
            owner: self,
            target: value,
        }
    }

    pub fn give(&self) -> Switch<'_> {
        Switch {
            owner: self,
            trigger: false,
        }
    }
    pub fn take(&self) -> Switch<'_> {
        Switch {
            owner: self,
            trigger: true,
        }
    }
}

pub struct Wait<'a> {
    owner: &'a AsyncFlag,
    target: bool,
}

impl<'a> Unpin for Wait<'a> {}

impl<'a> Future for Wait<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        self.owner.waker.register(cx.waker());

        if self.owner.value.load(Ordering::SeqCst) == self.target {
            Poll::Ready(())
        } else {
            Poll::Pending
        }
    }
}

pub struct Switch<'a> {
    owner: &'a AsyncFlag,
    trigger: bool,
}

impl<'a> Unpin for Switch<'a> {}

impl<'a> Future for Switch<'a> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        self.owner.waker.register(cx.waker());

        let ok = if !self.trigger {
            !self.owner.value.fetch_or(true, Ordering::SeqCst)
        } else {
            self.owner.value.fetch_and(false, Ordering::SeqCst)
        };

        if ok {
            Poll::Ready(())
        } else {
            Poll::Pending
        }
    }
}
