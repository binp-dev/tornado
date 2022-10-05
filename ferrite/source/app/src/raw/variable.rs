use super::import::*;
use futures::task::AtomicWaker;
use std::{
    ffi::CStr,
    ops::{Deref, DerefMut},
    os::raw::c_void,
    sync::atomic::{AtomicBool, Ordering},
    task::Waker,
};

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
        let ps = Box::new(ProcState::default());
        self.set_user_data(Box::into_raw(ps) as *mut c_void);
    }

    pub unsafe fn request_proc(&mut self) {
        log::trace!("PV('{:?}').request_proc()", self.name());
        let ps = self.proc_state();
        debug_assert!(!ps.requested(), "Variable '{:?}' already requested", self.name());
        ps.requested.store(true, Ordering::Release);
        fer_var_req_proc(self.ptr);
    }
    pub unsafe fn start_proc(&mut self) {
        log::trace!("PV('{:?}').start_proc()", self.name());
        let ps = self.proc_state();
        debug_assert!(!ps.processing(), "Variable '{:?}' already processing", self.name());
        ps.processing.store(true, Ordering::Release);
        ps.try_wake();
    }
    pub unsafe fn complete_proc(&mut self) {
        log::trace!("PV('{:?}').complete_proc()", self.name());
        let ps = self.proc_state();
        debug_assert!(ps.processing(), "Variable '{:?}' isn't processing", self.name());
        ps.processing.store(false, Ordering::Release);
        ps.requested.store(false, Ordering::Release);
        fer_var_proc_done(self.ptr);
    }

    pub unsafe fn lock(&self) {
        log::trace!("PV('{:?}').lock()", self.name());
        fer_var_lock(self.ptr);
    }
    pub unsafe fn unlock(&self) {
        log::trace!("PV('{:?}').unlock()", self.name());
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

    pub fn proc_state(&self) -> &ProcState {
        unsafe { (self.user_data() as *const ProcState).as_ref() }.unwrap()
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

    pub fn proc_state(&self) -> &'_ ProcState {
        self.var.proc_state()
    }
    pub fn name(&self) -> &CStr {
        self.var.name()
    }
    pub fn data_type(&self) -> Type {
        self.var.data_type()
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

#[derive(Default)]
pub(crate) struct ProcState {
    requested: AtomicBool,
    processing: AtomicBool,
    waker: AtomicWaker,
}

impl ProcState {
    pub fn requested(&self) -> bool {
        self.requested.load(Ordering::Acquire)
    }
    pub fn processing(&self) -> bool {
        self.processing.load(Ordering::Acquire)
    }

    pub fn set_waker(&self, waker: &Waker) {
        self.waker.register(waker);
    }
    pub fn clean_waker(&self) {
        self.waker.take();
    }
    pub fn try_wake(&self) {
        self.waker.wake();
    }
}

unsafe impl Send for ProcState {}
