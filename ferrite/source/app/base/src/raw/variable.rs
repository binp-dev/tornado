use super::import::*;
use std::{
    cell::Cell,
    ffi::CStr,
    ops::{Deref, DerefMut},
    os::raw::c_void,
    task::Waker,
};

pub use super::import::{
    FerVarDir as Dir, FerVarKind as Kind, FerVarScalarType as ScalarType, FerVarType as Type,
};

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
        let ps = self.proc_state_mut();
        if !ps.requested {
            ps.requested = true;
            fer_var_req_proc(self.ptr);
        }
    }
    pub unsafe fn start_proc(&mut self) {
        let ps = self.proc_state_mut();
        if !ps.processing {
            ps.processing = true;
            ps.try_wake();
        } else {
            panic!("Variable is already processing");
        }
    }
    pub unsafe fn complete_proc(&mut self) {
        let ps = self.proc_state_mut();
        ps.requested = false;
        ps.processing = false;
        fer_var_proc_done(self.ptr);
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

    pub fn proc_state(&self) -> &ProcState {
        unsafe { (self.user_data() as *const ProcState).as_ref() }.unwrap()
    }
    fn proc_state_mut(&mut self) -> &mut ProcState {
        unsafe { (self.user_data() as *mut ProcState).as_mut() }.unwrap()
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

    pub fn lock(&self) -> Guard<'_> {
        Guard::new(&self.var)
    }
    pub fn lock_mut(&mut self) -> GuardMut<'_> {
        GuardMut::new(&mut self.var)
    }
}

pub(crate) struct Guard<'a> {
    var: &'a VariableUnprotected,
}

impl<'a> Guard<'a> {
    fn new(var: &'a VariableUnprotected) -> Self {
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
impl<'a> Drop for Guard<'a> {
    fn drop(&mut self) {
        unsafe { self.var.unlock() };
    }
}

pub(crate) struct GuardMut<'a> {
    var: &'a mut VariableUnprotected,
}
impl<'a> GuardMut<'a> {
    fn new(var: &'a mut VariableUnprotected) -> Self {
        unsafe { var.lock() };
        Self { var }
    }
}
impl<'a> Deref for GuardMut<'a> {
    type Target = VariableUnprotected;
    fn deref(&self) -> &VariableUnprotected {
        self.var
    }
}
impl<'a> DerefMut for GuardMut<'a> {
    fn deref_mut(&mut self) -> &mut VariableUnprotected {
        self.var
    }
}
impl<'a> Drop for GuardMut<'a> {
    fn drop(&mut self) {
        unsafe { self.var.unlock() };
    }
}

#[derive(Default)]
pub(crate) struct ProcState {
    pub requested: bool,
    pub processing: bool,
    waker: Cell<Option<Waker>>,
}

impl ProcState {
    pub fn set_waker(&self, waker: &Waker) {
        self.waker.replace(Some(waker.clone()));
    }
    pub fn clean_waker(&self) {
        self.waker.take();
    }
    pub fn try_wake(&self) {
        if let Some(waker) = self.waker.take() {
            waker.wake();
        }
    }
}

unsafe impl Send for ProcState {}
