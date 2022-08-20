use super::import::*;
use std::{
    cell::UnsafeCell,
    ops::{Deref, DerefMut},
    os::raw::{c_char, c_void},
};

pub use super::import::{
    FerVarDir as Dir, FerVarKind as Kind, FerVarScalarType as ScalarType, FerVarType as Type,
};

pub struct Var {
    ptr: *mut FerVar,
}

unsafe impl Send for Var {}

impl Var {
    pub fn from_ptr(ptr: *mut FerVar) -> Self {
        Self { ptr }
    }

    pub unsafe fn request_proc(&mut self) {
        fer_var_request_proc(self.ptr)
    }
    pub unsafe fn proc_done(&mut self) {
        fer_var_proc_done(self.ptr)
    }
    pub unsafe fn lock(&mut self) {
        fer_var_lock(self.ptr)
    }
    pub unsafe fn unlock(&mut self) {
        fer_var_unlock(self.ptr)
    }

    pub unsafe fn name(&self) -> *const c_char {
        fer_var_name(self.ptr)
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

    pub unsafe fn user_data(&self) -> *const c_void {
        fer_var_user_data(self.ptr)
    }
    pub unsafe fn user_data_mut(&mut self) -> *mut c_void {
        fer_var_user_data(self.ptr)
    }
    pub unsafe fn set_user_data(&mut self, user_data: *mut c_void) {
        fer_var_set_user_data(self.ptr, user_data)
    }
}

pub struct Lock {
    var_cell: UnsafeCell<Var>,
}

unsafe impl Send for Lock {}

impl Lock {
    pub fn new(var: Var) -> Self {
        Self {
            var_cell: UnsafeCell::new(var),
        }
    }
    pub fn into_inner(self) -> Var {
        self.var_cell.into_inner()
    }

    pub unsafe fn lock(&self) -> Guard<'_> {
        let var_ptr = self.var_cell.get();
        // Lock before dereference to ensure that there is no mutable aliasing.
        (*var_ptr).lock();
        Guard::new(&mut *var_ptr)
    }
}

pub struct Guard<'a> {
    var: &'a mut Var,
}

impl<'a> Guard<'a> {
    fn new(var: &'a mut Var) -> Self {
        Self { var }
    }
}

impl<'a> Deref for Guard<'a> {
    type Target = Var;
    fn deref(&self) -> &Var {
        self.var
    }
}

impl<'a> DerefMut for Guard<'a> {
    fn deref_mut(&mut self) -> &mut Var {
        self.var
    }
}

impl<'a> Drop for Guard<'a> {
    fn drop(&mut self) {
        unsafe { self.var.unlock() };
    }
}
