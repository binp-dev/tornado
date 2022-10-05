use super::AsyncFlag;
use async_std::sync::{Arc, Mutex, MutexGuard};
use std::{
    mem::{swap, ManuallyDrop},
    ops::{Deref, DerefMut},
    ptr,
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
        self.write.ready.value()
    }
    pub async fn wait_ready(&self) {
        self.write.ready.wait(true).await
    }
    pub async fn try_swap(&mut self) -> bool {
        let mut guard = self.write.buffer.lock().await;
        if self.write.ready.try_take() {
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
    ready: &'a AsyncFlag,
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
        self.ready.try_give();
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
