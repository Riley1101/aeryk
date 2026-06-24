static long sys_open(const char *path) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(2), "D"(path)
                : "rcx", "r11", "memory");
  return ret;
}

static long sys_read(long fd, void *buf, long len) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(0), "D"(fd), "S"(buf), "d"(len)
                : "rcx", "r11", "memory");
  return ret;
}

static long sys_write(long fd, const void *buf, long len) {
  long ret;
  asm volatile("syscall"
                : "=a"(ret)
                : "0"(1), "D"(fd), "S"(buf), "d"(len)
                : "rcx", "r11", "memory");
  return ret;
}

static long str_len(const char *s) {
  long n = 0;
  while (s[n]) {
    n++;
  }
  return n;
}

int main(void) {
  const char *banner = "Hello from /bin/init (ring 3)!\n";
  sys_write(1, banner, str_len(banner));

  char buffer[128];
  long fd = sys_open("/hello.txt");
  if (fd >= 0) {
    long n = sys_read(fd, buffer, sizeof(buffer));
    if (n > 0) {
      sys_write(1, buffer, n);
    }
  }

  return 0;
}
