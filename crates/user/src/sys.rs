//! Hand-written `extern "C"` signatures for the userland libc functions
//! Rust userland programs call into (see libc/include/*.h). Mirrors the
//! kernel crate's crates/kernel/src/sys.rs bridge pattern: Rust owns the
//! logic, C owns the syscall ABI and existing tested runtime.

unsafe extern "C" {
    pub fn puts(s: *const u8) -> i32;
    pub fn exit(status: i32) -> !;
}
