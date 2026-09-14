#![no_std]

//! Compositor MVP: exclusively owns the real framebuffer (SYS_fbmap's
//! per-process lock enforces this -- see syscall.c), spawns a couple of
//! client processes (see aeryk_compositor_protocol for how, given this
//! kernel has no cross-exec shared memory yet), and redraws the whole
//! screen -- background, every active window, then the mouse cursor on
//! top -- each time a mouse packet arrives. Full-repaint rather than the
//! save/restore-under-cursor trick the first cursor-only version used:
//! once window content can change on its own (a client pushes a new
//! frame) independently of mouse movement, a saved-pixels cache goes
//! stale and starts painting over fresh window content, so this redraws
//! everything instead. Per-window damage tracking and a timer/vsync-driven
//! redraw (rather than only-on-mouse-motion) are separate, later README
//! items -- this is deliberately the simplest version that's still correct.

use aeryk_compositor_protocol::{WindowRequest, WindowSlot, MAX_WINDOWS, MAX_WIN_H, MAX_WIN_W};
use aeryk_user_rt::{
    fb::fbmap,
    mman::{mmap, MAP_ANONYMOUS, MAP_FAILED, MAP_SHARED, PROT_READ, PROT_WRITE},
    mouse::mouse_read,
    sys::{close, dup2, execve, exit, fork, pipe, puts, read, wait},
    FbInfo, MousePacket,
};

const BG_COLOR: u32 = 0xff282828; // matches tty.h's BG
const CURSOR_COLOR: u32 = 0xffebdbb2; // light foreground, visible on BG
const CURSOR_W: usize = 8;
const CURSOR_H: usize = 8;

// Classic arrow-ish pointer: one bit per pixel, MSB first, 8 rows.
const CURSOR_BITMAP: [u8; CURSOR_H] = [
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11100000,
    0b10110000,
    0b00011000,
];

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    unsafe { exit(101) }
}

fn log(msg: &str) {
    // puts() wants a NUL-terminated C string; msg comes from a &str
    // literal in this file, none of which contain interior NULs, so a
    // fixed-size stack copy + manual terminator is enough -- no
    // heap/allocator available in this no_std, no_alloc crate.
    let mut buf = [0u8; 128];
    let n = core::cmp::min(msg.len(), buf.len() - 1);
    buf[..n].copy_from_slice(&msg.as_bytes()[..n]);
    buf[n] = 0;
    unsafe {
        puts(buf.as_ptr());
    }
}

fn clamp(v: i32, lo: i32, hi: i32) -> i32 {
    if v < lo {
        lo
    } else if v > hi {
        hi
    } else {
        v
    }
}

fn paint_background(fb: *mut u32, stride: usize, width: usize, height: usize) {
    for y in 0..height {
        for x in 0..width {
            unsafe {
                *fb.add(x + y * stride) = BG_COLOR;
            }
        }
    }
}

/// Blits one window slot's pixels (tightly packed at its own `w`, not
/// MAX_WIN_W -- see protocol.rs) into the real framebuffer at its
/// position, clipping to the screen bounds. A no-op if the slot has no
/// client attached yet (`active == 0`).
fn blit_window(fb: *mut u32, stride: usize, fb_w: usize, fb_h: usize, slot: &WindowSlot) {
    if slot.active == 0 {
        return;
    }
    let w = slot.w as usize;
    let h = slot.h as usize;
    for row in 0..h {
        for col in 0..w {
            let px = slot.x + col as i32;
            let py = slot.y + row as i32;
            if px >= 0 && py >= 0 && (px as usize) < fb_w && (py as usize) < fb_h {
                let pixel = slot.pixels[row * w + col];
                unsafe {
                    *fb.add(px as usize + py as usize * stride) = pixel;
                }
            }
        }
    }
}

fn draw_cursor(fb: *mut u32, stride: usize, width: usize, height: usize, x: i32, y: i32) {
    for row in 0..CURSOR_H {
        let bits = CURSOR_BITMAP[row];
        for col in 0..CURSOR_W {
            if bits & (0x80 >> col) == 0 {
                continue;
            }
            let px = x + col as i32;
            let py = y + row as i32;
            if px >= 0 && py >= 0 && (px as usize) < width && (py as usize) < height {
                unsafe {
                    *fb.add(px as usize + py as usize * stride) = CURSOR_COLOR;
                }
            }
        }
    }
}

/// Spawns one client at (x, y): `path`/`arg` must be NUL-terminated. Forks
/// a proxy (shares `slot` via the mmap(MAP_SHARED) it inherits, since it's
/// a fork child of the compositor, not exec'd) which itself forks+execs
/// the real client over a pipe, reads its WindowRequest handshake, then
/// copies each frame the client writes straight into `slot`. See
/// protocol.rs module docs for why this indirection exists. Returns
/// immediately in the calling (compositor) process; the proxy and client
/// run and exit independently.
fn spawn_client(path: &[u8], arg: &[u8], x: i32, y: i32, slot: *mut WindowSlot) {
    let mut fds = [0i32; 2];
    if unsafe { pipe(fds.as_mut_ptr()) } != 0 {
        log("compositor: pipe() failed, skipping client");
        return;
    }

    let proxy_pid = unsafe { fork() };
    if proxy_pid < 0 {
        log("compositor: fork() failed, skipping client");
        return;
    }

    if proxy_pid == 0 {
        run_proxy(fds, path, arg, x, y, slot);
        // run_proxy always exits the process itself; unreachable.
    }

    // Compositor process: doesn't touch the pipe itself, only the proxy
    // and the client it spawns do.
    unsafe {
        close(fds[0]);
        close(fds[1]);
    }
}

/// Body of the forked proxy process. Never returns -- always exits.
fn run_proxy(fds: [i32; 2], path: &[u8], arg: &[u8], x: i32, y: i32, slot: *mut WindowSlot) -> ! {
    // Fork the real client *before* touching either end of the pipe here --
    // fork() duplicates the fd table as it stands at that instant, so
    // closing fds[1] first (as an earlier version of this did) would leave
    // the client with nothing valid to dup2() onto fd 3, and every
    // handshake read() below would just see immediate EOF.
    let client_pid = unsafe { fork() };
    if client_pid == 0 {
        unsafe {
            close(fds[0]);
            dup2(fds[1], 3);
            close(fds[1]);
            let argv: [*const u8; 3] = [path.as_ptr(), arg.as_ptr(), core::ptr::null()];
            execve(path.as_ptr(), argv.as_ptr());
            // Only reached if execve() failed.
            exit(1);
        }
    }

    // Proxy itself only ever reads fds[0]; the write end now belongs to
    // the client.
    unsafe {
        close(fds[1]);
    }

    let mut req = WindowRequest::zeroed();
    let req_size = core::mem::size_of::<WindowRequest>();
    let n = unsafe { read(fds[0], &mut req as *mut WindowRequest as *mut u8, req_size) };
    if n as usize != req_size {
        log("compositor: client proxy: handshake failed");
        unsafe { exit(1) };
    }

    let w = core::cmp::min(req.width as usize, MAX_WIN_W);
    let h = core::cmp::min(req.height as usize, MAX_WIN_H);
    unsafe {
        (*slot).x = x;
        (*slot).y = y;
        (*slot).w = w as u32;
        (*slot).h = h as u32;
        (*slot).active = 1;
    }

    let frame_bytes = w * h * 4;
    loop {
        let buf_ptr = unsafe { (*slot).pixels.as_mut_ptr() as *mut u8 };
        let mut got = 0usize;
        let mut ok = true;
        // A pipe read() can return fewer bytes than asked for even when
        // more are coming, so loop until a full frame (or EOF/error) --
        // same reasoning as any stream socket/pipe reader.
        while got < frame_bytes {
            let n = unsafe { read(fds[0], buf_ptr.add(got), frame_bytes - got) };
            if n <= 0 {
                ok = false;
                break;
            }
            got += n as usize;
        }
        if !ok {
            break;
        }
    }

    unsafe {
        (*slot).active = 0;
        close(fds[0]);
    }
    let mut status: i32 = 0;
    unsafe {
        wait(client_pid, &mut status as *mut i32);
        exit(0);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn main(_argc: i32, _argv: *const *const u8) -> i32 {
    let mut info = FbInfo::zeroed();
    let fb = unsafe { fbmap(&mut info as *mut FbInfo) };
    if fb == MAP_FAILED {
        log("compositor: fbmap failed (already owned by another process?)");
        unsafe { exit(1) };
    }
    let fb = fb as *mut u32;
    let width = info.width as usize;
    let height = info.height as usize;
    let stride = (info.pitch / 4) as usize;

    let slots_size = MAX_WINDOWS * core::mem::size_of::<WindowSlot>();
    let slots_ptr = unsafe {
        mmap(
            core::ptr::null_mut(),
            slots_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1,
            0,
        )
    };
    if slots_ptr == MAP_FAILED {
        log("compositor: mmap for window_slots failed");
        unsafe { exit(1) };
    }
    // SYS_mmap zeroes every page it hands back, so every WindowSlot (in
    // particular `active`) already starts at 0 -- no init loop needed.
    let slots = slots_ptr as *mut WindowSlot;

    spawn_client(b"/bin/testclient\0", b"1\0", 40, 40, unsafe { slots.add(0) });
    spawn_client(b"/bin/testclient\0", b"2\0", 400, 40, unsafe { slots.add(1) });
    log("compositor: mapped framebuffer, spawned clients, tracking mouse");

    let mut cx: i32 = (width / 2) as i32;
    let mut cy: i32 = (height / 2) as i32;

    let redraw = |cx: i32, cy: i32| {
        paint_background(fb, stride, width, height);
        for i in 0..MAX_WINDOWS {
            let slot = unsafe { &*slots.add(i) };
            blit_window(fb, stride, width, height, slot);
        }
        draw_cursor(fb, stride, width, height, cx, cy);
    };

    redraw(cx, cy);

    loop {
        let mut pkt = MousePacket::zeroed();
        let n = unsafe { mouse_read(&mut pkt as *mut MousePacket, 1) };
        if n != 1 {
            continue;
        }

        cx = clamp(cx + pkt.dx as i32, 0, width as i32 - 1);
        // PS/2 reports positive dy as "moved up"; screen y grows downward.
        cy = clamp(cy - pkt.dy as i32, 0, height as i32 - 1);

        redraw(cx, cy);
    }
}
