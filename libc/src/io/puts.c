#include <sys/stat.h>
#include <internal/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int puts(const char *s) {
    int ret = syscall(__NR_write, 1, s, strlen(s));
    ret = syscall(__NR_write, 1, "\n", 1);
    if ( ret < 0 )
        return -1;
    return ret;
}
