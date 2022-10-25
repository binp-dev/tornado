use atomic_enum::atomic_enum;
use futures::task::AtomicWaker;
use std::{
    ffi::CStr,
    ops::{Deref, DerefMut},
    os::raw::c_void,
    sync::atomic::Ordering,
    task::Waker,
};

use super::import::*;
pub use super::import::{FerVarDir as Dir, FerVarKind as Kind, FerVarScalarType as ScalarType, FerVarType as Type};

pub(crate) struct VariableUnprotected {
    ptr: *mut FerVar,
}

unsafe impl Send for VariableUnprotected {}

impl VariableUnprotected {
    pub unsafe fn from_ptr(ptr: *mut FerVar) -> Self {
        Self { ptr }
    }
    pub fn init(&mut self) {
        assert!(self.user_data().is_null());
        let info = Box::new(Info::new());
        self.set_user_data(Box::into_raw(info) as *mut c_void);
    }

    pub unsafe fn request_proc(&mut self) {
        let prev = self.info().swap_proc_state(ProcState::Requested);
        debug_assert_eq!(prev, ProcState::Idle);
        fer_var_request_proc(self.ptr);
    }
    pub unsafe fn proc_begin(&mut self) {
        let info = self.info();
        let prev = info.swap_proc_state(ProcState::Processing);
        debug_assert!(prev == ProcState::Idle || prev == ProcState::Requested);
        info.try_wake();
    }
    pub unsafe fn complete_proc(&mut self) {
        let prev = self.info().swap_proc_state(ProcState::Ready);
        debug_assert_eq!(prev, ProcState::Processing);
        fer_var_complete_proc(self.ptr);
    }
    pub unsafe fn proc_end(&mut self) {
        let info = self.info();
        let prev = info.swap_proc_state(ProcState::Complete);
        debug_assert_eq!(prev, ProcState::Ready);
        info.try_wake();
    }
    unsafe fn clean_proc(&mut self) {
        let prev = self.info().swap_proc_state(ProcState::Idle);
        debug_assert_eq!(prev, ProcState::Complete);
    }

    pub unsafe fn lock(&self) {
        fer_var_lock(self.ptr);
    }
    pub unsafe fn unlock(&self) {
        fer_var_unlock(self.ptr);
    }

    pub fn name(&self) -> &CStr {
        unsafe { CStr::from_ptr(fer_var_name(self.ptr)) }
    }
    pub fn data_type(&self) -> Type {
        unsafe { fer_var_type(self.ptr) }
    }

    pub fn data_ptr(&self) -> *const c_void {
        unsafe { fer_var_data(self.ptr) }
    }
    pub fn data_mut_ptr(&mut self) -> *mut c_void {
        unsafe { fer_var_data(self.ptr) }
    }
    pub fn array_len(&self) -> usize {
        unsafe { fer_var_array_len(self.ptr) }
    }
    pub fn array_set_len(&mut self, new_size: usize) {
        unsafe { fer_var_array_set_len(self.ptr, new_size) }
    }

    pub fn info(&self) -> &Info {
        unsafe { (self.user_data() as *const Info).as_ref() }.unwrap()
    }
    fn user_data(&self) -> *mut c_void {
        unsafe { fer_var_user_data(self.ptr) }
    }
    fn set_user_data(&mut self, user_data: *mut c_void) {
        unsafe { fer_var_set_user_data(self.ptr, user_data) }
    }
}

pub(crate) struct Variable {
    var: VariableUnprotected,
}

unsafe impl Send for Variable {}

impl Variable {
    pub unsafe fn new(var: VariableUnprotected) -> Self {
        Self { var }
    }
    #[allow(dead_code)]
    pub unsafe fn into_inner(self) -> VariableUnprotected {
        self.var
    }

    pub unsafe fn get_unprotected(&self) -> &VariableUnprotected {
        &self.var
    }
    pub unsafe fn get_unprotected_mut(&mut self) -> &mut VariableUnprotected {
        &mut self.var
    }

    pub fn lock(&mut self) -> Guard<'_> {
        Guard::new(&mut self.var)
    }

    pub fn info(&self) -> &'_ Info {
        self.var.info()
    }
    pub fn name(&self) -> &CStr {
        self.var.name()
    }
    pub fn data_type(&self) -> Type {
        self.var.data_type()
    }

    pub unsafe fn clean_proc(&mut self) {
        self.var.clean_proc()
    }
}

pub(crate) struct Guard<'a> {
    var: &'a mut VariableUnprotected,
}
impl<'a> Guard<'a> {
    fn new(var: &'a mut VariableUnprotected) -> Self {
        unsafe { var.lock() };
        Self { var }
    }
}
impl<'a> Deref for Guard<'a> {
    type Target = VariableUnprotected;
    fn deref(&self) -> &VariableUnprotected {
        self.var
    }
}
impl<'a> DerefMut for Guard<'a> {
    fn deref_mut(&mut self) -> &mut VariableUnprotected {
        self.var
    }
}
impl<'a> Drop for Guard<'a> {
    fn drop(&mut self) {
        unsafe { self.var.unlock() };
    }
}

#[atomic_enum]
#[derive(PartialEq)]
pub(crate) enum ProcState {
    Idle = 0,
    Requested,
    Processing,
    Ready,
    Complete,
}

pub(crate) struct Info {
    proc_state: AtomicProcState,
    waker: AtomicWaker,
}

impl Info {
    pub fn new() -> Self {
        Self {
            proc_state: AtomicProcState::new(ProcState::Idle),
            waker: AtomicWaker::new(),
        }
    }

    pub fn proc_state(&self) -> ProcState {
        self.proc_state.load(Ordering::Acquire)
    }
    fn swap_proc_state(&self, prev: ProcState) -> ProcState {
        self.proc_state.swap(prev, Ordering::SeqCst)
    }

    pub fn set_waker(&self, waker: &Waker) {
        self.waker.register(waker);
    }
    fn try_wake(&self) {
        self.waker.wake();
    }
}

unsafe impl Send for Info {}
