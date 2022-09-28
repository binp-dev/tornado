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
    pub fn split(self) -> (Reader<T>, Arc<Writer<T>>) {
        let write = Arc::new(Writer {
            buffer: Mutex::new(self.buffers.0),
            ready: AsyncFlag::new(false),
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
    ready: AsyncFlag,
}
impl<T> Writer<T> {
    pub async fn write(&self) -> MutexGuard<'_, Vec<T>> {
        self.buffer.lock().await
    }
}

pub struct Reader<T> {
    buffer: Vec<T>,
    write: Arc<Writer<T>>,
}
impl<T> Reader<T> {
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
impl<T: Clone> Reader<T> {
    pub fn into_stream(self) -> ReadStream<T> {
        ReadStream {
            buffer: self,
            pos: 0,
            cyclic: false,
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

pub struct ReadStream<T: Clone> {
    buffer: Reader<T>,
    pos: usize,
    cyclic: bool,
}
impl<T: Clone> ReadStream<T> {
    pub fn buffer(&self) -> &Reader<T> {
        &self.buffer
    }
    pub async fn next(&mut self) -> Option<T> {
        loop {
            if self.pos < self.buffer.len() {
                let value = self.buffer[self.pos].clone();
                self.pos += 1;
                break Some(value);
            } else if self.buffer.try_swap().await || self.cyclic {
                self.pos = 0;
            } else {
                break None;
            }
        }
    }
}
