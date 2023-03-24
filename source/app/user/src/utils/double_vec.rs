use async_atomic::{Atomic, Subscriber};
use std::{
    mem::swap,
    ops::{Deref, DerefMut},
    sync::Arc,
    sync::{Mutex, MutexGuard},
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
        let ready = Atomic::new(false).subscribe();
        let write = Arc::new(Writer {
            buffer: Mutex::new(self.buffers.0),
            ready: ready.clone(),
        });
        (
            Reader {
                buffer: self.buffers.1,
                write: write.clone(),
                ready,
            },
            write,
        )
    }
}

pub struct Writer<T> {
    buffer: Mutex<Vec<T>>,
    ready: Arc<Atomic<bool>>,
}
impl<T> Writer<T> {
    pub async fn write(&self) -> WriteGuard<'_, T> {
        WriteGuard {
            buffer: self.buffer.lock().unwrap(),
            ready: &self.ready,
        }
    }
}

pub struct Reader<T> {
    buffer: Vec<T>,
    write: Arc<Writer<T>>,
    ready: Subscriber<bool>,
}
impl<T> Reader<T> {
    pub async fn wait_ready(&mut self) {
        self.ready.wait(|x| x).await;
    }
    pub fn try_swap(&mut self) -> bool {
        if self.write.ready.swap(false) {
            self.buffer.clear();
            swap(
                self.write.buffer.lock().unwrap().deref_mut(),
                &mut self.buffer,
            );
            true
        } else {
            false
        }
    }
}
impl<T: Copy> Reader<T> {
    pub fn into_iter<M: ReadModifier>(self, modifier: M) -> ReadIterator<T, M> {
        ReadIterator::new(self, modifier)
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
    ready: &'a Atomic<bool>,
}
impl<'a, T> Drop for WriteGuard<'a, T> {
    fn drop(&mut self) {
        self.ready.swap(true);
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

pub trait ReadModifier {
    fn swap(&mut self);
    fn cyclic(&self) -> bool;
}

pub struct ReadIterator<T: Copy, M: ReadModifier> {
    buffer: Reader<T>,
    pos: usize,
    modifier: M,
}

impl<T: Copy, M: ReadModifier> ReadIterator<T, M> {
    fn new(buffer: Reader<T>, modifier: M) -> Self {
        ReadIterator {
            buffer,
            pos: 0,
            modifier,
        }
    }

    fn try_swap(&mut self) -> bool {
        if self.buffer.try_swap() {
            self.modifier.swap();
            true
        } else {
            false
        }
    }

    pub async fn wait_ready(&mut self) {
        if self.buffer.len() == 0 || (!self.modifier.cyclic() && self.pos >= self.buffer.len()) {
            self.buffer.wait_ready().await
        }
    }
}

impl<T: Copy, M: ReadModifier> Iterator for ReadIterator<T, M> {
    type Item = T;

    fn next(&mut self) -> Option<T> {
        loop {
            if self.pos < self.buffer.len() {
                let value = self.buffer[self.pos];
                self.pos += 1;
                break Some(value);
            } else if self.try_swap() || self.modifier.cyclic() {
                self.pos = 0;
            } else {
                break None;
            }
        }
    }
}
