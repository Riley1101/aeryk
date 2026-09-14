#include <stddef.h>
#include <string.h>
#include <utils.h>

#include <arch/x86_64/fs/vfs.h>
#include <arch/x86_64/drivers/keyboard.h>
#include <arch/x86_64/drivers/mouse.h>
#include <arch/x86_64/drivers/serial.h>

#include <abi/errno.h>
#include <abi/fb.h>
#include <abi/mman.h>
#include <pipe.h>
#include <pmm.h>
#include <process.h>
#include <stdint.h>
#include <syscall.h>
#include <timer.h>
#include <tty.h>
#include <usercopy.h>
#include <vmm.h>

/**
 * @brief Structure representing the state of registers during a system call.
 *
 * This structure is used to capture the values of registers when a user process
 * invokes a system call. It contains the values of general-purpose registers,
 * as well as the user stack pointer, flags, and instruction pointer.
 */
struct syscall_frame
{
    uint64_t rax;
    uint64_t r9, r8, r10, rdx, rsi, rdi;
    uint64_t user_rsp;
    uint64_t user_rflags;
    uint64_t user_rip;
    // Callee-saved registers, pushed by syscall_entry purely so fork() can
    // snapshot the caller's full context. Never restored from here on the
    // return path: the C ABI already keeps them intact across the call to
    // syscall_handler_c.
    uint64_t r15, r14, r13, r12, rbp, rbx;
};

#define MAX_ARG_LEN 128

/**
 * @brief Copies a NULL-terminated argv[] array from user space into
 * fixed-size kernel buffers, for SYS_execve. Each user pointer
 * is dereferenced directly rather than through a copy_from_user-style
 * safety wrapper, consistent with how every other syscall in this file
 * (SYS_open, SYS_read, ...) already trusts user pointers, since the
 * user's pages are mapped in the currently active pagetable during the
 * syscall.
 * @param user_argv NULL-terminated array of user pointers to NUL-terminated
 * strings, or NULL for an empty argv.
 * @param strs Kernel storage for up to MAX_USER_ARGS strings, each
 * truncated to MAX_ARG_LEN - 1 bytes.
 * @param argv Filled with pointers into `strs`, one per copied argument.
 * @return The number of arguments copied (0 if `user_argv` is NULL or
 * empty).
 */
static int copy_argv_from_user(const char *const *user_argv,
                               char strs[MAX_USER_ARGS][MAX_ARG_LEN],
                               char *argv[MAX_USER_ARGS])
{
    if (!user_argv)
    {
        return 0;
    }

    int argc = 0;
    for (; argc < MAX_USER_ARGS; argc++)
    {
        const char *user_str = user_argv[argc];
        if (!user_str)
        {
            break;
        }

        size_t i = 0;
        for (; i < MAX_ARG_LEN - 1 && user_str[i] != '\0'; i++)
        {
            strs[argc][i] = user_str[i];
        }
        strs[argc][i] = '\0';
        argv[argc] = strs[argc];
    }
    return argc;
}

/**
 * @brief Handles system calls invoked by user processes.
 *
 * This function is called when a user process invokes a system call using the `syscall` instruction.
 * It takes a pointer to a `syscall_frame` structure that contains the register values at
 * the time of the system call. The function processes the system call based on the value in the `rax` register,
 * which indicates the system call number. It supports various system calls such as reading from
 * the keyboard, writing to the console, opening and closing files, and exiting the process.
 */
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit
void syscall_handler_c(struct syscall_frame *frame)
{
    switch (frame->rax)
    {
    case SYS_read:
        if (frame->rdi == 0 && current_process->fd_table[0].type == FD_PIPE_READ)
        {
            // fd 0 was dup2()'d onto a pipe (shell redirection), so read
            // from that instead of the keyboard.
            frame->rax = pipe_read(current_process->fd_table[0].pipe,
                                    (char *)frame->rsi, (int)frame->rdx);
        }
        else if (frame->rdi == 0)
        {
            frame->rax = keyboard_read((char *)frame->rsi, (int)frame->rdx);
        }
        else if (frame->rdi >= 3 && frame->rdi < MAX_FDS)
        {
            file_descriptor_t *fd = &current_process->fd_table[frame->rdi];
            if (fd->type == FD_PIPE_READ)
            {
                // Same convention as keyboard_read() above: written
                // directly into the user pointer rather than through
                // copy_to_user(), since it runs in the current process's
                // own pagetable for the whole blocking loop.
                frame->rax = pipe_read(fd->pipe, (char *)frame->rsi, (int)frame->rdx);
            }
            else if (fd->type == FD_VFS && fd->node->type == VFS_FILE)
            {
                uint32_t bytes_to_read = frame->rdx;
                if (fd->offset + bytes_to_read > fd->node->size)
                {
                    bytes_to_read = fd->node->size - fd->offset;
                }
                if (bytes_to_read > 0)
                {
                    if (copy_to_user((void *)frame->rsi,
                                      (uint8_t *)fd->node->data + fd->offset,
                                      bytes_to_read) != 0)
                    {
                        frame->rax = (uint64_t)-EFAULT;
                        break;
                    }
                    fd->offset += bytes_to_read;
                }
                frame->rax = bytes_to_read;
            }
            else
            {
                frame->rax = (uint64_t)-EBADF;
            }
        }
        else
        {
            frame->rax = (uint64_t)-EBADF;
        }
        break;

    case SYS_write:
        if (frame->rdi == 1 && current_process->fd_table[1].type == FD_PIPE_WRITE)
        {
            // fd 1 was dup2()'d onto a pipe (shell redirection), so write
            // there instead of the console.
            frame->rax = pipe_write(current_process->fd_table[1].pipe,
                                     (const char *)frame->rsi, (int)frame->rdx);
        }
        else if (frame->rdi == 1)
        {
            print_n((const char *)frame->rsi, (size_t)frame->rdx);
            frame->rax = frame->rdx;
        }
        else if (frame->rdi >= 3 && frame->rdi < MAX_FDS &&
                 current_process->fd_table[frame->rdi].type == FD_PIPE_WRITE)
        {
            file_descriptor_t *fd = &current_process->fd_table[frame->rdi];
            frame->rax = pipe_write(fd->pipe, (const char *)frame->rsi, (int)frame->rdx);
        }
        else
        {
            frame->rax = (uint64_t)-EBADF;
        }
        break;
    case SYS_open:
    {
        const char *filename = (const char *)frame->rdi;
        vfs_node_t *file = vfs_find_node(vfs_root, filename);
        if (!file || file->type != VFS_FILE)
        {
            frame->rax = (uint64_t)-ENOENT;
            break;
        }

        int fd_index = -1;
        // starts at 3 because 0,1,2 for stdio
        for (int i = 3; i < MAX_FDS; i++)
        {
            if (current_process->fd_table[i].type == FD_NONE)
            {
                fd_index = i;
                break;
            }
        }

        if (fd_index != -1)
        {
            current_process->fd_table[fd_index].type = FD_VFS;
            current_process->fd_table[fd_index].node = file;
            current_process->fd_table[fd_index].offset = 0;
            current_process->fd_table[fd_index].flags = frame->rsi;
            frame->rax = fd_index;
        }
        else
        {
            frame->rax = (uint64_t)-EMFILE;
        }
        break;
    }
    case SYS_fork:
    {
        trapframe_t regs = {
            .rdi = frame->rdi,
            .rsi = frame->rsi,
            .rdx = frame->rdx,
            .r10 = frame->r10,
            .r8 = frame->r8,
            .r9 = frame->r9,
            .rbx = frame->rbx,
            .rbp = frame->rbp,
            .r12 = frame->r12,
            .r13 = frame->r13,
            .r14 = frame->r14,
            .r15 = frame->r15,
            .rax = frame->rax,
            .rip = frame->user_rip,
            .rflags = frame->user_rflags,
            .rsp = frame->user_rsp,
        };

        process_t *child = fork_process(current_process, &regs);
        frame->rax = child ? (int64_t)child->pid : (uint64_t)-ENOMEM;
        break;
    }
    case SYS_clone:
    {
        // Same trapframe replay as SYS_fork, just handed to clone_process()
        // with the flags/child-stack args instead. Matches the raw Linux
        // clone(2) syscall's arg order (flags in rdi, stack in rsi);
        // parent_tid/child_tid/tls (rdx/r10/r8) aren't implemented yet.
        trapframe_t regs = {
            .rdi = frame->rdi,
            .rsi = frame->rsi,
            .rdx = frame->rdx,
            .r10 = frame->r10,
            .r8 = frame->r8,
            .r9 = frame->r9,
            .rbx = frame->rbx,
            .rbp = frame->rbp,
            .r12 = frame->r12,
            .r13 = frame->r13,
            .r14 = frame->r14,
            .r15 = frame->r15,
            .rax = frame->rax,
            .rip = frame->user_rip,
            .rflags = frame->user_rflags,
            .rsp = frame->user_rsp,
        };

        process_t *child = clone_process(current_process, &regs, frame->rdi, frame->rsi);
        frame->rax = child ? (int64_t)child->pid : (uint64_t)-ENOMEM;
        break;
    }
    case SYS_execve:
    {
        const char *user_path = (const char *)frame->rdi;
        if (!user_path)
        {
            frame->rax = (uint64_t)-EFAULT;
            break;
        }

        char path[128];
        size_t i = 0;
        for (; i < sizeof(path) - 1 && user_path[i] != '\0'; i++)
        {
            path[i] = user_path[i];
        }
        path[i] = '\0';

        if (i == 0)
        {
            frame->rax = (uint64_t)-EINVAL;
            break;
        }

        char strs[MAX_USER_ARGS][MAX_ARG_LEN];
        char *argv[MAX_USER_ARGS];
        int argc = copy_argv_from_user((const char *const *)frame->rsi, strs, argv);
        if (argc == 0)
        {
            frame->rax = (uint64_t)-EINVAL;
            break;
        }

        // On success exec_process() jumps directly into the new program
        // and never returns here. Only the failure path sets rax (always
        // -1, e.g. file missing/not ELF/bad argv — see load_elf_into_new_pagetable).
        exec_process(current_process, path, argc, argv);
        frame->rax = (uint64_t)-ENOENT;
        break;
    }
    case SYS_readdir:
    {
        serial_print("[dbg] SYS_readdir enter\n");
        const char *path = (const char *)frame->rdi;
        char *buf = (char *)frame->rsi;
        uint32_t bufsize = (uint32_t)frame->rdx;

        vfs_node_t *dir = vfs_find_node(vfs_root, path);
        if (!dir)
        {
            frame->rax = (uint64_t)-ENOENT;
            break;
        }
        if (dir->type != VFS_DIRECTORY)
        {
            frame->rax = (uint64_t)-ENOTDIR;
            break;
        }

        // Built up in a kernel-side buffer and copied out in one shot at
        // the end, so a bad `buf` pointer only ever costs the final
        // copy_to_user() call rather than a partial, hard-to-diagnose
        // write into userland part way through the loop.
        char kbuf[512];
        if (bufsize > sizeof(kbuf))
        {
            bufsize = sizeof(kbuf);
        }

        uint32_t written = 0;
        for (vfs_node_t *child = dir->children; child; child = child->next)
        {
            size_t namelen = strlen(child->name);
            size_t needed = namelen + 1; // name + '\n' (plus optional '/')
            if (child->type == VFS_DIRECTORY)
            {
                needed++;
            }
            if (written + needed > bufsize)
            {
                break;
            }
            memcpy(kbuf + written, child->name, namelen);
            written += namelen;
            if (child->type == VFS_DIRECTORY)
            {
                kbuf[written++] = '/';
            }
            kbuf[written++] = '\n';
        }

        serial_print("[dbg] SYS_readdir before copy_to_user\n");
        if (written > 0 && copy_to_user(buf, kbuf, written) != 0)
        {
            serial_print("[dbg] SYS_readdir copy_to_user FAILED\n");
            frame->rax = (uint64_t)-EFAULT;
            break;
        }
        serial_print("[dbg] SYS_readdir after copy_to_user OK\n");
        frame->rax = written;
        break;
    }
    case SYS_wait:
    {
        int64_t pid_arg = (int64_t)frame->rdi;
        int *status_user = (int *)frame->rsi;
        int status = 0;
        int result;

        for (;;)
        {
            result = wait_reap_child(current_process, pid_arg, &status);
            if (result != 0)
            {
                break;
            }
            current_process->state = PROCESS_BLOCKED;
            schedule();
        }

        if (result > 0 && status_user &&
            copy_to_user(status_user, &status, sizeof(status)) != 0)
        {
            frame->rax = (uint64_t)-EFAULT;
            break;
        }
        // wait_reap_child() only ever returns -1 for "no children of ours
        // exist at all"; the blocking loop above already stops as soon as
        // it sees a non-zero result, so a negative result here is always
        // that ECHILD case, never some other error.
        frame->rax = (result < 0) ? (uint64_t)-ECHILD : (uint64_t)(int64_t)result;
        break;
    }
    case SYS_mmap:
    {
        // Register order matches the raw Linux mmap(2) syscall: addr, length,
        // prot, flags, fd, offset -- rdi, rsi, rdx, r10, r8, r9. `addr` is
        // ignored (this always picks the placement itself, like passing
        // addr=NULL on Linux); only anonymous mappings are supported, since
        // there's no page-cache to back a file-backed mapping with yet.
        uint64_t length = frame->rsi;
        int prot = (int)frame->rdx;
        int flags = (int)frame->r10;
        int64_t fd = (int64_t)frame->r8;

        if (length == 0 || fd != -1 || !(flags & MAP_ANONYMOUS) ||
            !(flags & (MAP_SHARED | MAP_PRIVATE)))
        {
            frame->rax = (uint64_t)-EINVAL;
            break;
        }

        uint64_t npages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t base = current_process->mmap_next;
        uint64_t *pml4 = (uint64_t *)(current_process->cr3 + hhdm_offset);

        uint64_t pte_flags = PTE_PRESENT | PTE_USER;
        if (prot & PROT_WRITE)
        {
            pte_flags |= PTE_WRITABLE;
        }
        if (!(prot & PROT_EXEC))
        {
            pte_flags |= PTE_NX;
        }
        if (flags & MAP_SHARED)
        {
            // Marks these pages so a later fork() shares the live frame
            // (still writable, still one physical page) with the child
            // instead of falling back to the usual COW-on-write split --
            // see PTE_SHARED in vmm.h. MAP_PRIVATE anonymous pages get no
            // such marker: an ordinary writable page, private to this
            // process, COW-split like any other on fork().
            pte_flags |= PTE_SHARED;
        }

        uint64_t mapped = 0;
        for (; mapped < npages; mapped++)
        {
            void *phys = pmm_alloc_page();
            if (!phys)
            {
                break;
            }
            memset((void *)((uint64_t)phys + hhdm_offset), 0, PAGE_SIZE);
            vmm_map_page(pml4, base + mapped * PAGE_SIZE, (uint64_t)phys, pte_flags);
        }

        if (mapped < npages)
        {
            frame->rax = (uint64_t)-ENOMEM;
            break;
        }

        current_process->mmap_next = base + npages * PAGE_SIZE;
        frame->rax = base;
        break;
    }
    case SYS_munmap:
    {
        uint64_t addr = frame->rdi;
        uint64_t length = frame->rsi;

        if (length == 0 || addr % PAGE_SIZE != 0 || addr < MMAP_BASE)
        {
            frame->rax = (uint64_t)-EINVAL;
            break;
        }

        uint64_t npages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t *pml4 = (uint64_t *)(current_process->cr3 + hhdm_offset);
        for (uint64_t i = 0; i < npages; i++)
        {
            vmm_unmap_page(pml4, addr + i * PAGE_SIZE);
        }
        frame->rax = 0;
        break;
    }
    case SYS_brk:
    {
        // Query form: SYS_brk(0) just reports the current break, matching
        // the userland sbrk(0) convention (no valid program ever legitimately
        // asks to set its break to address 0).
        uint64_t requested = frame->rdi;
        if (requested == 0)
        {
            frame->rax = current_process->brk;
            break;
        }

        if (requested < current_process->brk_start)
        {
            frame->rax = current_process->brk;
            break;
        }

        uint64_t old_top = (current_process->brk + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
        uint64_t new_top = (requested + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);

        if (new_top > old_top)
        {
            // Growing: map fresh zeroed pages to cover the new range. Pages
            // already mapped for the shrink case below are left in place
            // (not unmapped) rather than freed, so a shrink-then-grow within
            // the same page range doesn't need to re-fault/re-zero anything
            // an in-flight pointer might still reference.
            uint64_t *pml4 = (uint64_t *)(current_process->cr3 + hhdm_offset);
            for (uint64_t page = old_top; page < new_top; page += PAGE_SIZE)
            {
                void *phys = pmm_alloc_page();
                if (!phys)
                {
                    frame->rax = current_process->brk;
                    goto brk_done;
                }
                memset((void *)((uint64_t)phys + hhdm_offset), 0, PAGE_SIZE);
                vmm_map_page(pml4, page, (uint64_t)phys,
                             PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX);
            }
        }

        current_process->brk = requested;
        frame->rax = requested;
    brk_done:
        break;
    }
    case SYS_close:
        if (frame->rdi >= 3 && frame->rdi < MAX_FDS)
        {
            file_descriptor_t *fd = &current_process->fd_table[frame->rdi];
            if (fd->type == FD_PIPE_READ)
            {
                pipe_close_end(fd->pipe, 1);
            }
            else if (fd->type == FD_PIPE_WRITE)
            {
                pipe_close_end(fd->pipe, 0);
            }
            fd->type = FD_NONE;
            fd->node = NULL;
            fd->pipe = NULL;
            frame->rax = 0;
        }
        else
        {
            frame->rax = (uint64_t)-EBADF;
        }
        break;
    case SYS_pipe:
    {
        int *user_fds = (int *)frame->rdi;

        int read_fd = -1, write_fd = -1;
        for (int i = 3; i < MAX_FDS; i++)
        {
            if (current_process->fd_table[i].type == FD_NONE)
            {
                read_fd = i;
                break;
            }
        }
        for (int i = read_fd + 1; i < MAX_FDS; i++)
        {
            if (current_process->fd_table[i].type == FD_NONE)
            {
                write_fd = i;
                break;
            }
        }

        if (read_fd == -1 || write_fd == -1)
        {
            frame->rax = (uint64_t)-EMFILE;
            break;
        }

        pipe_t *p = pipe_create();
        if (!p)
        {
            frame->rax = (uint64_t)-ENOMEM;
            break;
        }

        current_process->fd_table[read_fd].type = FD_PIPE_READ;
        current_process->fd_table[read_fd].pipe = p;
        current_process->fd_table[write_fd].type = FD_PIPE_WRITE;
        current_process->fd_table[write_fd].pipe = p;

        int fds[2] = {read_fd, write_fd};
        if (copy_to_user(user_fds, fds, sizeof(fds)) != 0)
        {
            pipe_close_end(p, 1);
            pipe_close_end(p, 0);
            current_process->fd_table[read_fd].type = FD_NONE;
            current_process->fd_table[read_fd].pipe = NULL;
            current_process->fd_table[write_fd].type = FD_NONE;
            current_process->fd_table[write_fd].pipe = NULL;
            frame->rax = (uint64_t)-EFAULT;
            break;
        }
        frame->rax = 0;
        break;
    }
    case SYS_dup:
    {
        int oldfd = (int)frame->rdi;
        if (oldfd < 0 || oldfd >= MAX_FDS ||
            current_process->fd_table[oldfd].type == FD_NONE)
        {
            frame->rax = (uint64_t)-EBADF;
            break;
        }

        int newfd = -1;
        for (int i = 3; i < MAX_FDS; i++)
        {
            if (current_process->fd_table[i].type == FD_NONE)
            {
                newfd = i;
                break;
            }
        }
        if (newfd == -1)
        {
            frame->rax = (uint64_t)-EMFILE;
            break;
        }

        file_descriptor_t *src = &current_process->fd_table[oldfd];
        current_process->fd_table[newfd] = *src;
        if (src->type == FD_PIPE_READ)
        {
            src->pipe->readers++;
        }
        else if (src->type == FD_PIPE_WRITE)
        {
            src->pipe->writers++;
        }
        frame->rax = newfd;
        break;
    }
    case SYS_dup2:
    {
        int oldfd = (int)frame->rdi;
        int newfd = (int)frame->rsi;
        if (oldfd < 0 || oldfd >= MAX_FDS || newfd < 0 || newfd >= MAX_FDS ||
            current_process->fd_table[oldfd].type == FD_NONE)
        {
            frame->rax = (uint64_t)-EBADF;
            break;
        }

        if (newfd == oldfd)
        {
            frame->rax = newfd;
            break;
        }

        file_descriptor_t *dst = &current_process->fd_table[newfd];
        if (dst->type == FD_PIPE_READ)
        {
            pipe_close_end(dst->pipe, 1);
        }
        else if (dst->type == FD_PIPE_WRITE)
        {
            pipe_close_end(dst->pipe, 0);
        }

        file_descriptor_t *src = &current_process->fd_table[oldfd];
        *dst = *src;
        if (src->type == FD_PIPE_READ)
        {
            src->pipe->readers++;
        }
        else if (src->type == FD_PIPE_WRITE)
        {
            src->pipe->writers++;
        }
        frame->rax = newfd;
        break;
    }
    case SYS_mouse_read:
        frame->rax = mouse_read((mouse_packet_t *)frame->rdi, (int)frame->rsi);
        break;

    case SYS_fbmap:
    {
        // Maps the kernel's LFB into the caller's address space, out of the
        // same mmap_next bump region SYS_mmap uses. Unlike SYS_mmap, the
        // physical pages already exist (Limine's framebuffer, not
        // pmm_alloc_page()), so every mapped PTE is tagged PTE_NOPMM --
        // see vmm.h for why unmap/destroy/clone must never pmm_free_page()
        // or pmm_page_ref_inc() an LFB frame. Also tagged PTE_PWT for
        // write-combining (vmm_init_pat()) -- this mapping is written to
        // constantly and never read back, exactly the access pattern WC is
        // for; the kernel's own HHDM mapping of the same physical range
        // (used by tty.c) is untouched here, since Limine set that one up
        // before init_vmm() ever ran.
        FrameBuffer *fb = global_renderer->framebuffer;
        uint64_t phys_base = (uint64_t)fb->base_address - hhdm_offset;
        uint64_t size = fb->buffer_size;
        uint64_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

        uint64_t base = current_process->mmap_next;
        uint64_t *pml4 = (uint64_t *)(current_process->cr3 + hhdm_offset);

        uint64_t pte_flags =
            PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX | PTE_NOPMM | PTE_PWT;
        for (uint64_t i = 0; i < npages; i++)
        {
            vmm_map_page(pml4, base + i * PAGE_SIZE, phys_base + i * PAGE_SIZE, pte_flags);
        }
        current_process->mmap_next = base + npages * PAGE_SIZE;

        fb_info_t info = {
            .width = fb->width,
            .height = fb->height,
            .pitch = fb->pixels_per_scan_line * 4,
            .bpp = 32,
        };
        if (copy_to_user((void *)frame->rdi, &info, sizeof(info)) != 0)
        {
            frame->rax = (uint64_t)-EFAULT;
            break;
        }

        frame->rax = base;
        break;
    }

    case SYS_get_tsc_hz:
        // Lets userland convert its own rdtsc() deltas (a plain
        // instruction, no syscall needed to read the counter itself) into
        // real time -- see utils.h's rdtsc() and timer.h's tsc_hz for the
        // kernel-side half of this.
        frame->rax = tsc_hz;
        break;

    case SYS_exit:
        if (current_process)
        {
            process_release_fds(current_process);
            current_process->exit_code = (int)frame->rdi;
            current_process->state = PROCESS_DEAD;
            schedule();
        }

        for (;;)
        {
            asm volatile("hlt");
        }
        break;

    default:
        print("unknown syscall");
        frame->rax = (uint64_t)-ENOSYS;
        break;
    }
}

void init_syscalls(void)
{

    // Enable syscall extension bit
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);

    // 2. Configure STAR:
    // Bits 32-47: Kernel CS (0x08)
    // bits 48-63: Base for User CS/SS (0x10) -> SYSRET users 0x10+16 = 0x20
    // for CS, 0x10+8=0x18 for SS
    wrmsr(MSR_STAR, ((uint64_t)0x08 << 32) | ((uint64_t)0x10 << 48));

    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    wrmsr(MSR_FMASK, 0x200);
}
