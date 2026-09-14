#![no_std]

//! Client<->compositor wire protocol and the shared window-slot table.
//! Shared between crates/userland/compositor (the reader/writer via each
//! client's proxy) and crates/userland/testclient (the writer) -- previously copy-pasted
//! between the two; pulled out once a second client crate made the
//! duplication worth removing.
//!
//! There's no cross-exec shared memory in this kernel yet -- mmap(MAP_SHARED)
//! only stays shared across fork() (parent<->child), and execve() wipes the
//! calling process's entire user address space (see exec_process() in
//! process.c), so a client that's actually exec()'d a separate ELF can't
//! just inherit a pointer into the compositor's screen. Instead each client
//! talks to a small proxy process (forked from the compositor, so it shares
//! `window_slots` via ordinary MAP_SHARED-across-fork -- see shmtest.c for
//! the same trick) over a plain pipe:
//!
//!   1. Client writes one `WindowRequest` announcing its desired size.
//!   2. Client repeatedly writes a full `width * height` BGRA8888 frame
//!      whenever it wants to redraw (no partial-rect damage tracking yet --
//!      that's still a separate, later README item).
//!
//! The proxy copies each frame straight into the client's WindowSlot; the
//! compositor's main loop blits every active slot into the real framebuffer
//! each time it redraws.

/// Sent once by a client, first thing, over its protocol pipe.
#[repr(C)]
pub struct WindowRequest {
    pub width: u32,
    pub height: u32,
}

impl WindowRequest {
    pub const fn zeroed() -> Self {
        WindowRequest { width: 0, height: 0 }
    }
}

pub const MAX_WINDOWS: usize = 2;
pub const MAX_WIN_W: usize = 320;
pub const MAX_WIN_H: usize = 240;

/// One entry in the shared `window_slots` table (see module docs). Lives in
/// an mmap(MAP_SHARED|MAP_ANONYMOUS) region so both the compositor and each
/// client's proxy process (its fork child) can read/write it without any
/// new kernel-side sharing mechanism. `active`/`w`/`h` are read by the
/// compositor's redraw loop and written by the proxy, with no locking --
/// both sides only ever write fields that are meaningless until `active`
/// flips to 1, and a torn read of `w`/`h` on the very first frame is the
/// only race, which just skips one redraw at worst. Real synchronization
/// is future work (this MVP has no mutex/futex primitive yet either).
#[repr(C)]
pub struct WindowSlot {
    pub active: u32, // 0/1, not bool -- needs a defined plain-old-data layout
    pub x: i32,
    pub y: i32,
    pub w: u32,
    pub h: u32,
    pub pixels: [u32; MAX_WIN_W * MAX_WIN_H],
}
