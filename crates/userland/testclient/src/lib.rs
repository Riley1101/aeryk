#![no_std]

//! Minimal test client for the compositor MVP (see
//! aeryk_compositor_protocol for the wire format and why this talks over
//! a plain pipe instead of shared memory). Fills a buffer with a solid
//! color chosen by argv[1] ("1" or "2", anything else falls back to a
//! third color), sends the WindowRequest handshake, then pushes that
//! frame a few times with a delay between -- standing in for "the client
//! redraws periodically" until there's an actual reason to redraw
//! (animation, input) worth building.

use aeryk_compositor_protocol::WindowRequest;
use aeryk_user_rt::sys::{exit, malloc, write};

const WIDTH: u32 = 200;
const HEIGHT: u32 = 150;

fn pick_color(argv: *const *const u8) -> u32 {
    if argv.is_null() {
        return 0xff458588; // fallback: blue
    }
    let arg1 = unsafe { *argv.add(1) };
    if arg1.is_null() {
        return 0xff458588;
    }
    match unsafe { *arg1 } {
        b'1' => 0xffcc241d, // red
        b'2' => 0xff98971a, // green
        _ => 0xff458588,    // blue
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    unsafe { exit(101) }
}

#[unsafe(no_mangle)]
pub extern "C" fn main(_argc: i32, argv: *const *const u8) -> i32 {
    let color = pick_color(argv);

    let frame_bytes = (WIDTH * HEIGHT * 4) as usize;
    let buf = unsafe { malloc(frame_bytes) };
    if buf.is_null() {
        unsafe { exit(1) };
    }
    let pixels = buf as *mut u32;
    for i in 0..(WIDTH * HEIGHT) as usize {
        unsafe {
            *pixels.add(i) = color;
        }
    }

    let req = WindowRequest { width: WIDTH, height: HEIGHT };
    let req_size = core::mem::size_of::<WindowRequest>();
    let n = unsafe { write(3, &req as *const WindowRequest as *const u8, req_size) };
    if n as usize != req_size {
        unsafe { exit(1) };
    }

    for _ in 0..5 {
        let n = unsafe { write(3, buf, frame_bytes) };
        if n as usize != frame_bytes {
            break;
        }
        // No sleep syscall exists yet -- a busy-wait delay between
        // redraws, same pattern userland/fbtest.c already uses.
        for _ in 0..150_000_000i64 {
            unsafe {
                core::arch::asm!("nop");
            }
        }
    }

    unsafe { exit(0) };
}
