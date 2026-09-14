//! Hand-written `extern "C"` signatures for the C kernel functions Rust
//! modules call into. Mirrors kernel/src/arch/x86_64/include/{tty,pmm}.h.

use core::ffi::c_void;

unsafe extern "C" {
    pub fn print(str: *const u8);
    pub fn pmm_alloc_page() -> *mut c_void;
    pub fn pmm_free_page(page: *mut c_void);
    pub static hhdm_offset: u64;
}
