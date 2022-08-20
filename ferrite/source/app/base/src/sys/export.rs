use super::import::*;
use std::{thread, time::Duration};

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
