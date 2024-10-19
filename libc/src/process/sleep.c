#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec work, rem = {0, 0};
    work.tv_sec = seconds;
    int ret = nanosleep(&work, &rem);

    while ( rem.tv_sec != 0 || rem.tv_nsec != 0 ) {
        work = rem;
        nanosleep(&work, &rem);
    }
    if ( ret == 0 ) {
        return 0;
    }
    return -1;
}
