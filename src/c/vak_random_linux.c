#include "vak.h"

#include <unistd.h>
#include <stdlib.h>

int vak_seed_random(void)
{
    unsigned seed;

    if (getentropy(&seed, sizeof(seed)) < 0)
        return -1;
    srandom(seed);
    return 0;
}
