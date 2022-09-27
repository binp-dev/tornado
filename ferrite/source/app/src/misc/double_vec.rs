use async_std::sync::{Arc, Mutex, MutexGuard};
use std::{
    mem::swap,
    ops::{Deref, DerefMut},
    sync::atomic::{AtomicBool, Ordering},
};

pub struct DoubleVec<T> {
    buffers: (Vec<T>, Vec<T>),
}

impl<T> DoubleVec<T> {
    pub fn new(capacity: usize) -> Self {
        Self {
            buffers: (Vec::with_capacity(capacity), Vec::with_capacity(capacity)),
        }
    }
    pub fn split(self) -> (ReadVec<T>, Arc<WriteVec<T>>) {
        let write = Arc::new(WriteVec {
            buffer: Mutex::new(self.buffers.0),
            ready: AtomicBool::new(false),
        });
        (
            ReadVec {
                buffer: self.buffers.1,
                write: write.clone(),
            },
            write,
        )
    }
}

pub struct WriteVec<T> {
    buffer: Mutex<Vec<T>>,
    ready: AtomicBool,
}
pub struct ReadVec<T> {
    buffer: Vec<T>,
    write: Arc<WriteVec<T>>,
}

impl<T> WriteVec<T> {
    pub async fn lock(&self) -> MutexGuard<'_, Vec<T>> {
        self.buffer.lock().await
    }
    pub fn set_ready(&self) {
        self.ready.store(true, Ordering::Release);
    }
}

impl<T> ReadVec<T> {
    pub fn ready(&self) -> bool {
        self.write.ready.load(Ordering::Acquire)
    }
    pub async fn try_swap(&mut self) -> bool {
        if self.ready() {
            swap(self.write.lock().await.deref_mut(), &mut self.buffer);
            true
        } else {
            false
        }
    }
}
impl<T> Deref for ReadVec<T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Vec<T> {
        &self.buffer
    }
}
