use super::AsyncFlag;
use async_std::sync::{Arc, Mutex, MutexGuard};
use std::{
    mem::swap,
    ops::{Deref, DerefMut},
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
            ready: AsyncFlag::new(false),
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
    ready: AsyncFlag,
}

pub struct ReadVec<T> {
    buffer: Vec<T>,
    write: Arc<WriteVec<T>>,
}

impl<T> WriteVec<T> {
    pub async fn write(&self) -> MutexGuard<'_, Vec<T>> {
        self.buffer.lock().await
    }
}

impl<T> ReadVec<T> {
    pub fn ready(&self) -> bool {
        self.write.ready.get()
    }
    pub async fn wait_ready(&self) {
        self.write.ready.wait().await
    }
    pub async fn try_swap(&mut self) -> bool {
        let mut guard = self.write.buffer.lock().await;
        if self.write.ready.take() {
            swap(guard.deref_mut(), &mut self.buffer);
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

pub struct WriteGuard<'a, T> {
    inner: MutexGuard<'a, Vec<T>>,
    ready: &'a AsyncFlag,
}
impl<'a, T> Drop for WriteGuard<'a, T> {
    fn drop(&mut self) {
        self.ready.set();
    }
}
impl<'a, T> Deref for WriteGuard<'a, T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Vec<T> {
        self.inner.deref()
    }
}
impl<'a, T> DerefMut for WriteGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut Vec<T> {
        self.inner.deref_mut()
    }
}
