#ifndef TIMING_H
#define TIMING_H

#include <time.h>

static inline double wall_time_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#endif /* TIMING_H */
