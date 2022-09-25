use async_std::sync::{Arc, Mutex, MutexGuard};
use std::{
    mem::swap,
    ops::{Deref, DerefMut},
};

pub struct DoubleVec<T> {
    bufs: (Vec<T>, Vec<T>),
    ready: bool,
}
impl<T> DoubleVec<T> {
    pub fn new(capacity: usize) -> Self {
        Self {
            bufs: (Vec::with_capacity(capacity), Vec::with_capacity(capacity)),
            ready: false,
        }
    }
    pub fn split(self) -> (ReadVec<T>, WriteVec<T>) {
        let base = Arc::new(Mutex::new(self));
        (ReadVec { base: base.clone() }, WriteVec { base })
    }
    pub fn swap(&mut self) {
        swap(&mut self.bufs.0, &mut self.bufs.1);
    }
}

pub struct ReadVec<T> {
    base: Arc<Mutex<DoubleVec<T>>>,
}
impl<T> ReadVec<T> {
    pub async fn lock(&mut self) -> ReadVecGuard<'_, T> {
        ReadVecGuard {
            guard: self.base.lock().await,
        }
    }
}

pub struct WriteVec<T> {
    base: Arc<Mutex<DoubleVec<T>>>,
}
impl<T> WriteVec<T> {
    pub async fn lock(&mut self) -> WriteVecGuard<'_, T> {
        WriteVecGuard {
            guard: self.base.lock().await,
        }
    }
}

pub struct ReadVecGuard<'a, T> {
    guard: MutexGuard<'a, DoubleVec<T>>,
}
impl<'a, T> ReadVecGuard<'a, T> {
    pub fn set_ready(&mut self) {
        self.guard.ready = true;
    }
}
impl<'a, T> Deref for ReadVecGuard<'a, T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Self::Target {
        &self.guard.bufs.0
    }
}

pub struct WriteVecGuard<'a, T> {
    guard: MutexGuard<'a, DoubleVec<T>>,
}
impl<'a, T> WriteVecGuard<'a, T> {
    pub fn ready(&self) -> bool {
        self.guard.ready
    }
    pub fn try_swap(&mut self) -> bool {
        if self.guard.ready {
            self.guard.swap();
            self.guard.ready = false;
            true
        } else {
            false
        }
    }
}
impl<'a, T> Deref for WriteVecGuard<'a, T> {
    type Target = Vec<T>;
    fn deref(&self) -> &Self::Target {
        &self.guard.bufs.1
    }
}
impl<'a, T> DerefMut for WriteVecGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.guard.bufs.1
    }
}
