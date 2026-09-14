//! Mirrors abi/include/abi/fb.h.

/// Mirrors abi/include/abi/fb.h's fb_info_t.
#[repr(C)]
pub struct FbInfo {
    pub width: u32,
    pub height: u32,
    pub pitch: u32,
    pub bpp: u32,
}

impl FbInfo {
    pub const fn zeroed() -> Self {
        FbInfo { width: 0, height: 0, pitch: 0, bpp: 0 }
    }
}

unsafe extern "C" {
    pub fn fbmap(info: *mut FbInfo) -> *mut u8;
}
