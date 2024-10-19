#ifndef TIME_H
#define TIME_H

struct time_tv {
    long tv_sec;
    long tv_usec;
};

struct timespec {
    struct time_tv tv_sec;
    long tv_nsec;
};

#endif
