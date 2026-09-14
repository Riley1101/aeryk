#![no_std]

//! Shared `extern "C"` bridge into the userland libc (see libc/include/*.h)
//! for every Rust userland program on aeryk (rusthello, compositor,
//! testclient, ...). Rust owns program logic, C owns the syscall ABI and
//! the already-tested runtime (crt0.o, libc.a) -- same split as
//! crates/kernel/src/sys.rs uses for the kernel side. Pulled out of each
//! program's own crate (where it used to be copy-pasted three times) once
//! there were enough of them for the duplication to cost more than a
//! shared dependency does.

pub mod fb;
pub mod mman;
pub mod mouse;
pub mod sys;

pub use fb::FbInfo;
pub use mman::MAP_FAILED;
pub use mouse::MousePacket;
