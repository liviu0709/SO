// SPDX-License-Identifier: BSD-3-Clause

#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <internal/syscall.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	/* TODO: Implement mmap(). */
    struct stat idk;
    if ( fd > 0 && fstat(fd, &idk) < 0 ) {
        return MAP_FAILED;
    }

    if ( ((flags & MAP_PRIVATE) == 0) && ((flags & MAP_SHARED) == 0) ) {
        errno = EINVAL;
        return MAP_FAILED;
    }

    void *ret = (void*)syscall(__NR_mmap, addr, length, prot, flags, fd, offset);
    if ( ret == MAP_FAILED ) {
        return MAP_FAILED;
    }
	return ret;
}

void *mremap(void *old_address, size_t old_size, size_t new_size, int flags)
{
	/* TODO: Implement mremap(). */
    void *ret = (void*)syscall(__NR_mremap, old_address, old_size, new_size, flags);
    if ( ret == MAP_FAILED ) {
	    errno = 1;
        return MAP_FAILED;
    }
    return ret;
}

int munmap(void *addr, size_t length)
{
	/* TODO: Implement munmap(). */
    int ret = syscall(__NR_munmap, addr, length);
    if ( ret < 0 ) {
        errno = EINVAL;
        return -1;
    }
	return 0;
}
