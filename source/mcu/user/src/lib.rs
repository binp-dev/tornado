#![no_std]

#[cfg(feature = "real")]
#[macro_use]
mod real;
#[cfg(feature = "real")]
use real::*;

#[cfg(feature = "emul")]
#[macro_use]
mod emul;
#[cfg(feature = "emul")]
use emul::*;

#[no_mangle]
pub extern "C" fn user_main() {
    println!("Hello from Rust!\n");
    assert_eq!(1, 2);
}
