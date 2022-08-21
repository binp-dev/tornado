use super::import::*;
use lazy_static::lazy_static;
use std::{
    cell::UnsafeCell,
    collections::HashMap,
    ffi::{CStr, CString},
    ops::{Deref, DerefMut},
    os::raw::c_void,
    sync::Mutex,
};

pub use super::import::{
    FerVarDir as Dir, FerVarKind as Kind, FerVarScalarType as ScalarType, FerVarType as Type,
};

pub(crate) struct Var {
    ptr: *mut FerVar,
}

unsafe impl Send for Var {}

impl Var {
    pub unsafe fn new(ptr: *mut FerVar) -> Self {
        Self { ptr }
    }

    pub unsafe fn request_proc(&mut self) {
        fer_var_request_proc(self.ptr)
    }
    pub unsafe fn proc_done(&mut self) {
        fer_var_proc_done(self.ptr)
    }
    unsafe fn lock(&mut self) {
        fer_var_lock(self.ptr)
    }
    unsafe fn unlock(&mut self) {
        fer_var_unlock(self.ptr)
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

    pub unsafe fn user_data(&self) -> *const UserData {
        fer_var_user_data(self.ptr) as *const UserData
    }
    pub unsafe fn user_data_mut(&mut self) -> *mut UserData {
        fer_var_user_data(self.ptr) as *mut UserData
    }
    unsafe fn set_user_data(&mut self, user_data: *mut UserData) {
        fer_var_set_user_data(self.ptr, user_data as *mut c_void)
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

pub(crate) struct UserData {}

unsafe impl Send for UserData {}
