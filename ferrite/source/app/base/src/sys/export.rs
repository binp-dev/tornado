use super::{import::*, var::Var};
use crate::variable::{registry, AnyVariable};
use std::panic::{self, PanicInfo};

#[no_mangle]
pub extern "C" fn fer_app_init() {
    println!("fer_app_init()");
    let old_hook = panic::take_hook();
    panic::set_hook(Box::new(move |info: &PanicInfo| {
        old_hook(info);
        unsafe { fer_app_exit() };
    }))
}

#[no_mangle]
pub extern "C" fn fer_app_start() {
    println!("fer_app_start()");
}

#[no_mangle]
pub extern "C" fn fer_var_init(ptr: *mut FerVar) {
    let var = unsafe { AnyVariable::new(Var::new(ptr)) };
    println!("fer_var_init({})", var.name());
    registry::add_variable(var);
}
#[no_mangle]
pub extern "C" fn fer_var_proc_start(ptr: *mut FerVar) {
    println!("fer_var_proc_start({:?})", ptr);
    let var_type = unsafe { fer_var_type(ptr) };
    assert_eq!(var_type.kind, FerVarKind::Scalar);
    assert_eq!(var_type.scalar_type, FerVarScalarType::I32);
    unsafe {
        let val_ptr = fer_var_data(ptr) as *mut i32;
        if var_type.dir == FerVarDir::Write {
            *val_ptr += 1;
        }
        println!("    value: {}", *val_ptr);
        fer_var_proc_done(ptr);
    }
}
