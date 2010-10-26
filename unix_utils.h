#ifndef UNIX_UTILS_H_
#define UNIX_UTILS_H_

#include <time.h>

#define timestamp_t             struct timespec
#define init_timestamper        noop
#define get_timestamp(t)        clock_gettime(CLOCK_MONOTONIC, &t)
#define timestamp_diff(t1,t2)   timespec_diff(&t1, &t2)
#define PRINTF_TIMESTAMP_STR    "%d.%09ld"
#define PRINTF_TIMESTAMP_VAL(t) (int)t.tv_sec, t.tv_nsec

static inline void noop(void) {}

#define ONE_BILLION 1000000000

static inline void timespec_diff(struct timespec* tp1, const struct timespec* tp2)
{
  tp1->tv_sec = tp2->tv_sec - tp1->tv_sec;
  tp1->tv_nsec = tp2->tv_nsec - tp1->tv_nsec;
  if (tp1->tv_nsec < 0)
  {
    tp1->tv_sec--;
    tp1->tv_nsec = ONE_BILLION + tp1->tv_nsec;
  }
}

#endif /* UNIX_UTILS_H_ */
