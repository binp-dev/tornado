#![no_std]

use core::panic::PanicInfo;

extern "C" {
    fn __hal_panic() -> !;
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    unsafe { __hal_panic() }
}

#[no_mangle]
pub extern "C" fn hello() {
    panic!();
}
