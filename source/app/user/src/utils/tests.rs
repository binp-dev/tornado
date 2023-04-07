use futures::{pin_mut, poll};
use std::{
    future::Future,
    ops::{Deref, DerefMut},
    pin::Pin,
    sync::atomic::{AtomicUsize, Ordering},
    task::{Context, Poll},
    time::Duration,
};
use tokio::{join, sync::Mutex, test as async_test, time::sleep};

struct Counter {
    value: AtomicUsize,
}
impl Counter {
    pub fn new() -> Self {
        Counter {
            value: AtomicUsize::new(0),
        }
    }
    pub fn value(&self) -> usize {
        self.value.load(Ordering::SeqCst)
    }
    pub fn add(&self, value: usize) {
        self.value.fetch_add(value, Ordering::SeqCst);
    }
    pub fn future(&self) -> CounterFuture<'_> {
        CounterFuture { owner: self }
    }
}
struct CounterFuture<'a> {
    owner: &'a Counter,
}
impl<'a> Unpin for CounterFuture<'a> {}
impl<'a> Future for CounterFuture<'a> {
    type Output = ();
    fn poll(self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<()> {
        self.owner.add(1);
        Poll::Pending
    }
}

#[async_test]
async fn spurious_wakeup() {
    let (a, b) = (Counter::new(), Counter::new());
    let ab = async { join!(a.future(), b.future()) };

    assert_eq!((a.value(), b.value()), (0, 0));
    pin_mut!(ab);
    assert_eq!(poll(ab).await, Poll::Pending);

    assert_eq!((a.value(), b.value()), (1, 1));
}

#[async_test]
async fn mutex() {
    let mutex = Mutex::new(0);
    let (a, b) = (Counter::new(), Counter::new());

    join!(
        async {
            {
                let mut guard = mutex.lock().await;

                assert_eq!(*guard.deref(), 0);
                assert_eq!((a.value(), b.value()), (0, 0));
                a.add(1);

                sleep(Duration::from_millis(100)).await;

                *guard.deref_mut() += 1;
                a.add(1);
                assert_eq!((a.value(), b.value()), (2, 0));
            }

            sleep(Duration::from_millis(100)).await;

            assert_eq!((a.value(), b.value()), (2, 2));
            assert_eq!(*mutex.lock().await.deref(), 2);
        },
        async {
            sleep(Duration::from_millis(10)).await;

            {
                let mut guard = mutex.lock().await;

                assert_eq!(*guard.deref(), 1);
                assert_eq!((a.value(), b.value()), (2, 0));
                b.add(1);
                *guard.deref_mut() += 1;
                b.add(1);
                assert_eq!((a.value(), b.value()), (2, 2));
            }
        }
    );
}
