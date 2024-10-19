#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec work;
    work.tv_sec.tv_sec = seconds;
    int ret = nanosleep(&work, NULL);
    if ( ret == 0 )
        return 0;
    return -1;
}