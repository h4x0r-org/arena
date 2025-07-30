#pragma once

#include <stdbool.h>

#include <time.h>
#include <sys/time.h>

#define h4x0r_save_timestamp(x) clock_gettime(CLOCK_REALTIME, x)
#define H4X0R_NSEC_PER_SEC      1000000000

static inline bool
h4x0r_timespec_lt(struct timespec *t1, struct timespec *t2)
{
    if (t1->tv_sec < t2->tv_sec) {
        return true;
    }

    if (t1->tv_sec > t2->tv_sec) {
        return false;
    }

    return t1->tv_nsec < t2->tv_nsec;
}

static inline void
h4x0r_timespec_diff(struct timespec *item1,
                    struct timespec *item2,
                    struct timespec *result)
{
    int64_t          tmp;
    struct timespec *smaller, *bigger;

    if (h4x0r_timespec_lt(item1, item2)) {
        smaller = item1;
        bigger  = item2;
    }
    else {
        smaller = item2;
        bigger  = item1;
    }

    result->tv_sec = bigger->tv_sec - smaller->tv_sec;

    tmp = bigger->tv_nsec - smaller->tv_nsec;

    if (tmp < 0) {
        tmp += H4X0R_NSEC_PER_SEC;
        result->tv_sec -= 1;
    }

    result->tv_nsec = tmp;
}
