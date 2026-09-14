#![no_std]

use core::panic::PanicInfo;

mod slab;
mod sys;

use sys::print;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    unsafe {
        print(c"[!] RUST PANIC\n".as_ptr().cast());
    }
    loop {}
}