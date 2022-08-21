use base::{entry_point, variable::AnyVariable};
use macro_rules_attribute::apply;
use std::collections::HashMap;

pub use base::export;

#[apply(entry_point)]
fn app_main(registry: HashMap<String, AnyVariable>) {
    for name in registry.keys() {
        println!("{}", name);
    }
}
