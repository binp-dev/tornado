use async_atomic::{AsyncAtomic, AtomicSubscriber};
use futures::lock::{Mutex, MutexGuard};
use std::{
    mem::{swap, ManuallyDrop},
    ops::{Deref, DerefMut},
    ptr,
    sync::Arc,
};

type AtomicFlag = AtomicSubscriber<bool, Arc<AsyncAtomic<bool>>>;

pub struct DoubleVec<T> {
    buffers: (Vec<T>, Vec<T>),
}
impl<T> DoubleVec<T> {
    pub fn new(capacity: usize) -> Self {
        Self {
            buffers: (Vec::with_capacity(capacity), Vec::with_capacity(capacity)),
        }
    }
    pub fn split(self) -> (Reader<T>, Arc<Writer<T>>) {
        let write = Arc::new(Writer {
            buffer: Mutex::new(self.buffers.0),
            ready: AsyncAtomic::new(false).split().1,
        });
        (
            Reader {
                buffer: self.buffers.1,
                write: write.clone(),
            },
            write,
        )
    }
}

pub struct Writer<T> {
    buffer: Mutex<Vec<T>>,
    ready: AtomicFlag,
}
impl<T> Writer<T> {
    pub async fn write(&self) -> WriteGuard<'_, T> {
        WriteGuard {
            buffer: self.buffer.lock().await,
            ready: &self.ready,
        }
    }
}

pub struct Reader<T> {
    buffer: Vec<T>,
    write: Arc<Writer<T>>,
}
impl<T> Reader<T> {
    pub fn ready(&self) -> bool {
        self.write.ready.load()
    }
    pub async fn wait_ready(&self) {
        self.write.ready.wait(|x| x).await;
    }
    pub async fn try_swap(&mut self) -> bool {
        let mut guard = self.write.buffer.lock().await;
        if self.write.ready.fetch_and(false) {
            self.buffer.clear();
            swap(guard.deref_mut(), &mut self.buffer);
            true
        } else {
            false
        }
    }
}

impl<T> Deref for Reader<T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Vec<T> {
        &self.buffer
    }
}

pub struct WriteGuard<'a, T> {
    buffer: MutexGuard<'a, Vec<T>>,
    ready: &'a AsyncAtomic<bool>,
}
impl<'a, T> WriteGuard<'a, T> {
    pub fn discard(mut self) {
        self.buffer.clear();
        let mut self_ = ManuallyDrop::new(self);
        unsafe { ptr::drop_in_place(&mut self_.buffer as *mut MutexGuard<'a, _>) };
    }
}
impl<'a, T> Drop for WriteGuard<'a, T> {
    fn drop(&mut self) {
        self.ready.fetch_or(true);
    }
}
impl<'a, T> Deref for WriteGuard<'a, T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Vec<T> {
        self.buffer.deref()
    }
}
impl<'a, T> DerefMut for WriteGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut Vec<T> {
        self.buffer.deref_mut()
    }
}

pub struct ReaderStream<T: Clone> {
    buffer: Reader<T>,
    pos: usize,
    pub cyclic: bool,
    pub on_swap: Box<dyn FnMut()>,
}

impl<T: Clone> ReaderStream<T> {
    fn new(buffer: Reader<T>) -> Self {
        ReaderStream {
            buffer,
            pos: 0,
            cyclic: false,
            on_swap: Box::new(|| ()),
        }
    }

    async fn try_swap(&mut self) -> bool {
        //log::info!("try swap");
        if self.buffer.try_swap().await {
            (self.on_swap)();
            true
        } else {
            false
        }
    }
    pub async fn next(&mut self) -> Option<T> {
        loop {
            if self.pos < self.buffer.len() {
                let value = self.buffer[self.pos].clone();
                self.pos += 1;
                break Some(value);
            } else if self.try_swap().await || self.cyclic {
                self.pos = 0;
            } else {
                break None;
            }
        }
    }
    pub async fn wait_ready(&mut self) {
        self.buffer.wait_ready().await
    }
    pub fn len(&self) -> usize {
        self.buffer.len() - self.pos
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}
