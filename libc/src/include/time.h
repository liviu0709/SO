#ifndef TIME_H
#define TIME_H

struct timespec {
    long tv_sec;
    long tv_nsec;
};

int nanosleep(struct timespec* wait, struct timespec* rem);

#endif
