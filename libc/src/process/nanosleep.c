#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

int nanosleep(struct timespec* wait, struct timespec* rem) {
    int ret = syscall(__NR_nanosleep, wait, rem);
    if ( rem != NULL ) {
        while ( rem->tv_sec != 0 || rem->tv_nsec != 0 ) {
            wait->tv_nsec = rem->tv_nsec;
            wait->tv_sec = rem->tv_sec;
            nanosleep(wait, rem);
        }
    }
    if ( ret == 0 ) {
        return 0;
    }
    errno = EINTR;
    return -1;
}
