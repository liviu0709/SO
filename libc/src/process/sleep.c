#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec work, rem = {0, 0};
    work.tv_sec = seconds;
    // Compiler bad and wont let me this isnt set at 0....
    work.tv_nsec = 0;
    int ret = nanosleep(&work, &rem);

    if ( ret == 0 ) {
        return 0;
    }
    return -1;
}
