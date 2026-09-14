#ifndef _SYS_FB_H
#define _SYS_FB_H 1

#include <abi/fb.h>
#include <sys/mman.h> // MAP_FAILED

/**
 * @brief Maps the kernel's framebuffer into this process's address space
 * and fills `info` with its geometry.
 * @param info Destination for the framebuffer's width/height/pitch/bpp.
 * @return Pointer to the start of the mapped framebuffer (write BGRA/RGBA
 * pixels directly, striding by `info->pitch` bytes per row), or
 * MAP_FAILED on error (errno set). Unmap with munmap(ptr, height * pitch)
 * when done.
 */
void *fbmap(fb_info_t *info);

#endif // !_SYS_FB_H
