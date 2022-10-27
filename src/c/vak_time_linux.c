#include "vak.h"

#include <stdlib.h>
#include <sys/time.h>

/** Get the wall time.
 *
 * This functions uses the same representation of time as the vrt
 * functions, i.e. the number of microseconds the epoch which is
 * midnight 1970-01-01.
 *
 * \returns The number of microseconds since the epoch.
 */
vak_time_t vak_get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t t =
        (uint64_t)(tv.tv_sec) * 1000000 +
        (uint64_t)(tv.tv_usec);
    return t;
}

int vak_adjust_time(vak_time_t adj)
{
    vak_time_t t64;
    struct timeval tv;

    t64 = vak_get_time();
    t64 += adj;

    tv.tv_sec = t64 / 1000000;
    tv.tv_usec = t64 % 1000000;

    return settimeofday(&tv, NULL);
}

