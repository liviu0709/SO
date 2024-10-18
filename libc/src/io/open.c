// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <internal/syscall.h>
#include <stdarg.h>
#include <errno.h>

int open(const char *filename, int flags, ...)
{
	/* TODO: Implement open system call. */
    va_list args;
    __builtin_va_start(args, flags);
    int mode = __builtin_va_arg(args, int);
    __builtin_va_end(args);

    int ret = syscall(__NR_open, filename, flags, mode);
    if ( ret < 0 ) {
        errno = -ret;
        return -1;
    }
    return ret;
}
