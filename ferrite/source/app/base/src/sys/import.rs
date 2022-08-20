use std::os::raw::{c_char, c_void};

#[repr(C)]
pub struct FerVar {
    _unused: [u8; 0],
}

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FerVarKind {
    Scalar,
    Array,
}

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FerVarDir {
    Read,
    Write,
}

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FerVarScalarType {
    None,
    U8,
    I8,
    U16,
    I16,
    U32,
    I32,
    U64,
    I64,
    F32,
    F64,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct FerVarType {
    pub kind: FerVarKind,
    pub dir: FerVarDir,
    pub scalar_type: FerVarScalarType,
    pub array_max_len: usize,
}

extern "C" {
    pub fn fer_var_request_proc(var: *mut FerVar);
    pub fn fer_var_proc_done(var: *mut FerVar);
    pub fn fer_var_lock(var: *mut FerVar);
    pub fn fer_var_unlock(var: *mut FerVar);

    pub fn fer_var_name(var: *mut FerVar) -> *const c_char;
    pub fn fer_var_type(var: *mut FerVar) -> FerVarType;

    pub fn fer_var_data(var: *mut FerVar) -> *mut c_void;
    pub fn fer_var_array_len(var: *mut FerVar) -> usize;
    pub fn fer_var_array_set_len(var: *mut FerVar, new_size: usize);

    pub fn fer_var_user_data(var: *mut FerVar) -> *mut c_void;
    pub fn fer_var_set_user_data(var: *mut FerVar, user_data: *mut c_void);
}
