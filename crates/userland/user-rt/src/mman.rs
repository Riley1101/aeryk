//! Mirrors abi/include/abi/mman.h.

pub const PROT_READ: i32 = 0x1;
pub const PROT_WRITE: i32 = 0x2;
pub const MAP_SHARED: i32 = 0x01;
pub const MAP_ANONYMOUS: i32 = 0x20;

/// `(void *)-1` -- libc's MAP_FAILED. fbmap() shares this failure
/// convention too.
pub const MAP_FAILED: *mut u8 = usize::MAX as *mut u8;

unsafe extern "C" {
    pub fn mmap(
        addr: *mut u8,
        length: usize,
        prot: i32,
        flags: i32,
        fd: i32,
        offset: i64,
    ) -> *mut u8;
}
