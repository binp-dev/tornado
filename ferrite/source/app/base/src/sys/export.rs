use super::{import::*, var::Var};
use crate::variable::{registry, AnyVariable};
use std::{
    collections::HashMap,
    panic::{self, PanicInfo},
};

extern "Rust" {
    pub fn ferrite_app_main(variables: HashMap<String, AnyVariable>);
}

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
    unsafe { ferrite_app_main(registry::take()) };
}

#[no_mangle]
pub extern "C" fn fer_var_init(ptr: *mut FerVar) {
    let mut raw = Var::from_ptr(ptr);
    unsafe { raw.init() };
    let var = unsafe { AnyVariable::new(raw) };
    println!("fer_var_init({})", var.name());
    registry::add_variable(var);
}

#[no_mangle]
pub extern "C" fn fer_var_proc_start(ptr: *mut FerVar) {
    // No need for lock here - variable is already locked during this call.
    let mut raw = Var::from_ptr(ptr);
    unsafe { raw.proc_start() };
}
