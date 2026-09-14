#![no_std]

//! Scaffolding for Rust userland programs on aeryk. This crate builds to a
//! staticlib linked in place of a program's own .c.o, alongside the
//! existing C crt0.o + libc.a (see GNUmakefile's rust-user-lib rule) --
//! crt0.asm already just does `call main`, so a Rust-exported `main`
//! slots in with no changes to the boot path or linker script. Mirrors
//! crates/kernel's split: Rust owns the logic, C owns the syscall ABI and
//! the already-tested runtime (see crates/userland/user-rt).
//!
//! Today this just proves the toolchain end to end (build, link, boot,
//! run) with a trivial program; compositor/testclient are the real UI
//! code that followed once this pipeline was proven.

use aeryk_user_rt::sys::{exit, puts};

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    unsafe { exit(101) }
}

#[unsafe(no_mangle)]
pub extern "C" fn main(_argc: i32, _argv: *const *const u8) -> i32 {
    unsafe {
        puts(b"hello from rust\0".as_ptr());
        exit(0);
    }
}
