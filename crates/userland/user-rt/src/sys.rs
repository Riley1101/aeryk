//! General process/IO syscalls, mirroring libc/include/{stdio,stdlib,unistd}.h.

unsafe extern "C" {
    pub fn puts(s: *const u8) -> i32;
    pub fn exit(status: i32) -> !;
    pub fn malloc(size: usize) -> *mut u8;

    pub fn read(fd: i32, buf: *mut u8, count: usize) -> i64;
    pub fn write(fd: i32, buf: *const u8, count: usize) -> i64;
    pub fn pipe(fds: *mut i32) -> i32;
    pub fn close(fd: i32) -> i32;
    pub fn dup2(oldfd: i32, newfd: i32) -> i32;
    pub fn fork() -> i32;
    pub fn execve(path: *const u8, argv: *const *const u8) -> i32;
    pub fn wait(pid: i32, status: *mut i32) -> i32;
    pub fn sleep_ms(ms: u32) -> i32;
}
