//! Mirrors abi/include/abi/mouse.h.

/// Mirrors abi/include/abi/mouse.h's mouse_packet_t.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct MousePacket {
    pub dx: i16,
    pub dy: i16,
    pub buttons: u8,
}

impl MousePacket {
    pub const fn zeroed() -> Self {
        MousePacket { dx: 0, dy: 0, buttons: 0 }
    }
}

unsafe extern "C" {
    pub fn mouse_read(buf: *mut MousePacket, max_packets: i32) -> i32;
}
