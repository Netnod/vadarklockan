#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/time.h>

#include "vak.h"
#include "vrt.h"

int main(int argc, char *argv[]) {
    int r;
    overlap_value_t lo, hi;

    r = vak_main(&lo, &hi);
    if (r < 0) {
        fprintf(stderr, "vak_main error\n");
    } else if (r == 0) {
        fprintf(stderr, "vak_main gave up on getting time\n");
    } else {
        // use midpoint as adjustment
        double adj = (lo + hi) / 2;
        printf("vak_main succeded, time adjustment range %.3f .. %.3f\n", lo, hi);
        if (vak_adjust_time(adj * 1000000) < 0) {
            fprintf(stderr, "vak_adjust_time failed: %s\n", strerror(errno));
        }
    }

    exit(0);
}
