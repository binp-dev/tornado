use super::any::AnyVariable;
use lazy_static::lazy_static;
use std::{collections::HashMap, sync::Mutex};

lazy_static! {
    pub static ref REGISTRY: Mutex<HashMap<String, AnyVariable>> = Mutex::new(HashMap::new());
}

pub(crate) fn add_variable(var: AnyVariable) {
    assert!(REGISTRY.lock().unwrap().insert(var.name(), var).is_none());
}
