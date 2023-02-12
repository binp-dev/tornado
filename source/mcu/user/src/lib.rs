#![no_std]

mod error;
pub use error::Error;

#[cfg(feature = "real")]
#[macro_use]
mod real;
#[cfg(feature = "real")]
pub use real::*;

#[cfg(feature = "emul")]
#[macro_use]
mod emul;
#[cfg(feature = "emul")]
pub use emul::*;

#[no_mangle]
pub extern "C" fn user_main() {
    println!("Hello from Rust!\n");
    assert_eq!(1, 2);
}
