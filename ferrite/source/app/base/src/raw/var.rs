use super::import::*;
use std::{
    cell::Cell,
    cell::UnsafeCell,
    ffi::CStr,
    ops::{Deref, DerefMut},
    os::raw::c_void,
    task::Waker,
};

pub use super::import::{
    FerVarDir as Dir, FerVarKind as Kind, FerVarScalarType as ScalarType, FerVarType as Type,
};

pub(crate) struct Var {
    ptr: *mut FerVar,
}

unsafe impl Send for Var {}

impl Var {
    pub fn from_ptr(ptr: *mut FerVar) -> Self {
        Self { ptr }
    }
    pub unsafe fn init(&mut self) {
        let ps = Box::new(ProcState::default());
        self.set_user_data(Box::into_raw(ps) as *mut c_void);
    }

    pub unsafe fn req_proc(&mut self) {
        let ps = self.proc_state_mut();
        if !ps.requested {
            ps.requested = true;
            fer_var_req_proc(self.ptr);
        }
    }
    pub unsafe fn proc_start(&mut self) {
        let ps = self.proc_state_mut();
        if !ps.processing {
            ps.processing = true;
            ps.try_wake();
        } else {
            panic!("Variable is already processing");
        }
    }
    pub unsafe fn proc_done(&mut self) {
        let ps = self.proc_state_mut();
        ps.requested = false;
        ps.processing = false;
        fer_var_proc_done(self.ptr);
    }

    unsafe fn lock(&mut self) {
        fer_var_lock(self.ptr);
    }
    unsafe fn unlock(&mut self) {
        fer_var_unlock(self.ptr);
    }

    pub unsafe fn name(&self) -> &CStr {
        CStr::from_ptr(fer_var_name(self.ptr))
    }
    pub unsafe fn type_(&self) -> Type {
        fer_var_type(self.ptr)
    }

    pub unsafe fn data(&self) -> *const c_void {
        fer_var_data(self.ptr)
    }
    pub unsafe fn data_mut(&mut self) -> *mut c_void {
        fer_var_data(self.ptr)
    }
    pub unsafe fn array_len(&self) -> usize {
        fer_var_array_len(self.ptr)
    }
    pub unsafe fn array_set_len(&mut self, new_size: usize) {
        fer_var_array_set_len(self.ptr, new_size)
    }

    pub unsafe fn proc_state(&self) -> &ProcState {
        &*(self.user_data() as *const ProcState)
    }
    unsafe fn proc_state_mut(&mut self) -> &mut ProcState {
        &mut *(self.user_data() as *mut ProcState)
    }
    unsafe fn user_data(&self) -> *mut c_void {
        fer_var_user_data(self.ptr)
    }
    unsafe fn set_user_data(&mut self, user_data: *mut c_void) {
        fer_var_set_user_data(self.ptr, user_data)
    }
}

pub(crate) struct VarLock {
    var_cell: UnsafeCell<Var>,
}

unsafe impl Send for VarLock {}

impl VarLock {
    pub fn new(var: Var) -> Self {
        Self {
            var_cell: UnsafeCell::new(var),
        }
    }
    pub fn into_inner(self) -> Var {
        self.var_cell.into_inner()
    }

    pub unsafe fn lock(&self) -> VarGuard<'_> {
        let var_ptr = self.var_cell.get();
        // VarLock before dereference to ensure that there is no mutable aliasing.
        (*var_ptr).lock();
        VarGuard::new(&mut *var_ptr)
    }
}

pub(crate) struct VarGuard<'a> {
    var: &'a mut Var,
}

impl<'a> VarGuard<'a> {
    fn new(var: &'a mut Var) -> Self {
        Self { var }
    }
}

impl<'a> Deref for VarGuard<'a> {
    type Target = Var;
    fn deref(&self) -> &Var {
        self.var
    }
}

impl<'a> DerefMut for VarGuard<'a> {
    fn deref_mut(&mut self) -> &mut Var {
        self.var
    }
}

impl<'a> Drop for VarGuard<'a> {
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
