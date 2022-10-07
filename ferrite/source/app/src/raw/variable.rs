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
        let ps = Box::new(Info::new());
        self.set_user_data(Box::into_raw(ps) as *mut c_void);
    }

    pub unsafe fn request_proc(&mut self) {
        log::trace!("PV({:?}).request_proc()", self.name());
        let info = self.info();
        let ps = info.proc_state();
        debug_assert_eq!(ps, ProcState::Idle);
        info.set_proc_state(ProcState::Requested);
        fer_var_request_proc(self.ptr);
    }
    pub unsafe fn proc_begin(&mut self) {
        log::trace!("PV({:?}).proc_begin()", self.name());
        let info = self.info();
        let ps = info.proc_state();
        debug_assert!(ps == ProcState::Idle || ps == ProcState::Requested);
        info.set_proc_state(ProcState::Processing);
        info.try_wake();
    }
    pub unsafe fn complete_proc(&mut self) {
        log::trace!("PV({:?}).complete_proc()", self.name());
        let info = self.info();
        let ps = info.proc_state();
        debug_assert_eq!(ps, ProcState::Processing);
        info.set_proc_state(ProcState::Ready);
        fer_var_complete_proc(self.ptr);
    }
    pub unsafe fn proc_end(&mut self) {
        log::trace!("PV({:?}).proc_end()", self.name());
        let info = self.info();
        let ps = info.proc_state();
        debug_assert_eq!(ps, ProcState::Ready);
        info.set_proc_state(ProcState::Complete);
        info.try_wake();
    }
    pub unsafe fn clean_proc(&mut self) {
        log::trace!("PV({:?}).clean_proc()", self.name());
        let info = self.info();
        let ps = info.proc_state();
        debug_assert_eq!(ps, ProcState::Complete);
        info.set_proc_state(ProcState::Idle);
    }

    pub unsafe fn lock(&self) {
        log::trace!("PV({:?}).lock()", self.name());
        fer_var_lock(self.ptr);
    }
    pub unsafe fn unlock(&self) {
        log::trace!("PV({:?}).unlock()", self.name());
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
    fn set_proc_state(&self, ps: ProcState) {
        self.proc_state.store(ps, Ordering::Release);
    }

    pub fn set_waker(&self, waker: &Waker) {
        self.waker.register(waker);
    }
    fn try_wake(&self) {
        self.waker.wake();
    }
}

unsafe impl Send for Info {}
