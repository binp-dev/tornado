mod sys;
pub mod variable;

pub use sys::export;

#[macro_export]
macro_rules! entry_point {
    (
        $(#[$fn_meta:meta])*
        $fn_vis:vis fn $fn_name:ident($( $arg_name:ident : $arg_type:ty ),* $(,)?)
        $fn_body:block
    ) => (
        $(#[$fn_meta])*
        $fn_vis fn $fn_name($( $arg_name : $arg_type, )*)
        $fn_body

        #[no_mangle]
        pub extern "Rust" fn ferrite_app_main(registry: ::std::collections::HashMap<::std::string::String, $crate::variable::AnyVariable>) {
            $fn_name(registry)
        }
    );
}

pub fn add(left: usize, right: usize) -> usize {
    left + right
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }
}
