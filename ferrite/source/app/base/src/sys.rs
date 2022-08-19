use std::os::raw::{c_char, c_void};
use std::{thread, time::Duration};

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

#[no_mangle]
pub extern "C" fn fer_app_init() {
    println!("fer_app_init()");
}
#[no_mangle]
pub extern "C" fn fer_app_start() {
    println!("fer_app_start()");
}

#[no_mangle]
pub extern "C" fn fer_var_init(var: *mut FerVar) {
    println!("fer_var_init({:?})", var);
    let var_val = var as usize;
    thread::spawn(move || loop {
        let var = var_val as *mut FerVar;
        thread::sleep(Duration::from_secs(2));
        unsafe { fer_var_request_proc(var) };
    });
}
#[no_mangle]
pub extern "C" fn fer_var_proc_start(var: *mut FerVar) {
    println!("fer_var_proc_start({:?})", var);
    let var_type = unsafe { fer_var_type(var) };
    assert_eq!(var_type.kind, FerVarKind::Scalar);
    assert_eq!(var_type.scalar_type, FerVarScalarType::I32);
    unsafe {
        let val_ptr = fer_var_data(var) as *mut i32;
        if var_type.dir == FerVarDir::Write {
            *val_ptr += 1;
        }
        println!("    value: {}", *val_ptr);
        fer_var_proc_done(var);
    }
}
