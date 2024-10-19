#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int nanosleep(struct timespec* wait, struct timespec* rem) {
    int ret = syscall(__NR_nanosleep, wait, rem);
    if ( ret == 0 ) {
        return 0;
    }
    errno = EINTR;
    return -1;
}
