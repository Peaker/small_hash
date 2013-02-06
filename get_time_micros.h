#ifndef __GET_TIME_MICROS_H_
#define __GET_TIME_MICROS_H_

#include <stdint.h>
#include <sys/time.h>           /* gettimeofday */
#include <stdio.h>              /* perror */
#include <stdlib.h>             /* abort */

static inline uint64_t get_time_micros() {
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    if(0 != rc) {
        perror("gettimeofday");
        abort();
    }
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

#endif
