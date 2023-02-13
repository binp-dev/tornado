use crate::println;
use core::panic::PanicInfo;

extern "C" {
    fn __hal_panic() -> !;
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    let _ = println!("PANIC: {}", info);
    unsafe { __hal_panic() }
}
