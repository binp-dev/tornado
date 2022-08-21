use super::any::AnyVariable;
use lazy_static::lazy_static;
use std::{collections::HashMap, mem, sync::Mutex};

lazy_static! {
    static ref REGISTRY: Mutex<HashMap<String, AnyVariable>> = Mutex::new(HashMap::new());
}

pub(crate) fn add_variable(var: AnyVariable) {
    assert!(REGISTRY.lock().unwrap().insert(var.name(), var).is_none());
}

pub(crate) fn take() -> HashMap<String, AnyVariable> {
    let mut ret = HashMap::new();
    mem::swap(&mut *REGISTRY.lock().unwrap(), &mut ret);
    ret
}
