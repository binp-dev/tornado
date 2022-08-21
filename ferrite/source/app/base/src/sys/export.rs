use super::{
    import::*,
    var::{ReadStage, Stage, Var, WriteStage},
};
use crate::variable::{registry, AnyVariable};
use std::{
    collections::HashMap,
    panic::{self, PanicInfo},
    sync::atomic::Ordering,
};

#[no_mangle]
pub extern "C" fn fer_app_init() {
    println!("fer_app_init()");
    let old_hook = panic::take_hook();
    panic::set_hook(Box::new(move |info: &PanicInfo| {
        old_hook(info);
        unsafe { fer_app_exit() };
    }))
}

extern "Rust" {
    pub fn ferrite_app_main(variables: HashMap<String, AnyVariable>);
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
    let raw = Var::from_ptr(ptr);
    if let Some(user_data) = unsafe { raw.user_data().as_ref() } {
        match &user_data.stage {
            Stage::Read(read_stage) => match read_stage.load(Ordering::SeqCst) {
                ReadStage::Idle => {
                    read_stage.store(ReadStage::Processing, Ordering::SeqCst);
                    user_data.waker.wake();
                }
                ReadStage::Processing => {
                    panic!("Read is already processing");
                }
            },
            Stage::Write(write_stage) => match write_stage.load(Ordering::SeqCst) {
                WriteStage::Idle => {
                    panic!("Write hasn't been requested");
                }
                WriteStage::Requested => {
                    write_stage.store(WriteStage::Processing, Ordering::SeqCst);
                    user_data.waker.wake();
                }
                WriteStage::Processing => {
                    panic!("Write is already processing");
                }
            },
        }
    }
}
