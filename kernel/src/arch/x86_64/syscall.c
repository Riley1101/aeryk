#include <stddef.h>
#include <string.h>
#include <utils.h>

#include <arch/x86_64/fs/vfs.h>
#include <arch/x86_64/drivers/keyboard.h>
#include <arch/x86_64/drivers/serial.h>

#include <pipe.h>
#include <process.h>
#include <stdint.h>
#include <syscall.h>
#include <tty.h>
#include <usercopy.h>

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
                        frame->rax = -1;
                        break;
                    }
                    fd->offset += bytes_to_read;
                }
                frame->rax = bytes_to_read;
            }
            else
            {
                frame->rax = -1;
            }
        }
        else
        {
            frame->rax = -1;
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
            frame->rax = -1;
        }
        break;
    case SYS_open:
    {
        const char *filename = (const char *)frame->rdi;
        vfs_node_t *file = vfs_find_node(vfs_root, filename);
        if (!file || file->type != VFS_FILE)
        {
            frame->rax = -1; // file not found
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
            frame->rax = -1;
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
        frame->rax = child ? (int64_t)child->pid : -1;
        break;
    }
    case SYS_execve:
    {
        const char *user_path = (const char *)frame->rdi;
        if (!user_path)
        {
            frame->rax = -1;
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
            frame->rax = -1;
            break;
        }

        char strs[MAX_USER_ARGS][MAX_ARG_LEN];
        char *argv[MAX_USER_ARGS];
        int argc = copy_argv_from_user((const char *const *)frame->rsi, strs, argv);
        if (argc == 0)
        {
            frame->rax = -1;
            break;
        }

        // On success exec_process() jumps directly into the new program
        // and never returns here. Only the failure path sets rax.
        exec_process(current_process, path, argc, argv);
        frame->rax = -1;
        break;
    }
    case SYS_readdir:
    {
        serial_print("[dbg] SYS_readdir enter\n");
        const char *path = (const char *)frame->rdi;
        char *buf = (char *)frame->rsi;
        uint32_t bufsize = (uint32_t)frame->rdx;

        vfs_node_t *dir = vfs_find_node(vfs_root, path);
        if (!dir || dir->type != VFS_DIRECTORY)
        {
            frame->rax = -1;
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
            frame->rax = -1;
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
            frame->rax = -1;
            break;
        }
        frame->rax = (uint64_t)(int64_t)result;
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
            frame->rax = -1;
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
            frame->rax = -1;
            break;
        }

        pipe_t *p = pipe_create();
        if (!p)
        {
            frame->rax = -1;
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
            frame->rax = -1;
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
            frame->rax = -1;
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
            frame->rax = -1;
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
            frame->rax = -1;
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
    case SYS_exit:
        print("\n[Syscall] Process exited.\n");

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
        frame->rax = -1;
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
