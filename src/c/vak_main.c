#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "vak.h"

#include "overlap_algo.h"

/* Number of overlapping responses required to succeed */
static const int WANTED_OVERLAPS = 10;

/* Maximum uncertainty (seconds) of overlap required to succeed */
static const double WANTED_UNCERTAINTY = 2.0;

int vak_main(overlap_value_t *plo, overlap_value_t *phi)
{
    struct vak_server **servers, **server;
    struct overlap_algo *algo;
    int success = 0;
    int nr_responses = 0;
    int nr_overlaps;

    overlap_value_t lo, hi;

    servers = vak_get_randomized_servers();
    if (!servers) {
        fprintf(stderr, "vak_get_randomized_servers failed\n");
        return 0;
    }

    /* Create the overlap algorithm */
    algo = overlap_new();
    if (!algo) {
        fprintf(stderr, "overlap_new failed\n");
        vak_del_servers(servers);
        return 0;
    }

    /* Query the randomized list of servers until we have a majority
     * of responses which all overlap, the number of overlaps is
     * enough and the uncertainty is low enough */
    for (server = servers; *server; server++) {
        if (!vak_query_server(*server, &lo, &hi))
            continue;

        nr_responses++;

        overlap_add(algo, lo, hi);
        nr_overlaps = overlap_find(algo, &lo, &hi);

        if (nr_overlaps > nr_responses / 2 &&
            nr_overlaps >= WANTED_OVERLAPS &&
            (hi - lo) <= WANTED_UNCERTAINTY) {
            success = 1;
            break;
        }
    }

    /* Delete the server list and overlap algorithm since we're
     * finished with them. */
    vak_del_servers(servers);
    overlap_del(algo);

    /* Exit with an error if we didn't succeed */
    if (!success) {
        printf("failure: unable to get %d overlapping responses\n", WANTED_OVERLAPS);
        return 0;
    }

    printf("success: %d/%d overlapping responses, range %.6f .. %.6f\n",
           nr_overlaps, nr_responses, lo, hi);

    *plo = lo;
    *phi = hi;

    return 1;
}

