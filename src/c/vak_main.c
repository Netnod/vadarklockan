#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "vak.h"

#include "overlap_algo.h"

int vak_main(overlap_value_t *plo, overlap_value_t *phi)
{
    struct vak_server const **servers = NULL;
    struct vak_udp *udp = NULL;
    struct vak_impl *impl = NULL;
    int r = -1;

    if (vak_seed_random() < 0) {
        fprintf(stderr, "vak_seed_random failed: %s\n", strerror(errno));
        goto out;
    }

    servers = vak_get_randomized_servers();
    if (!servers) {
        fprintf(stderr, "vak_get_randomized_servers failed\n");
        goto out;
    }

    udp = vak_udp_new();
    if (!udp) {
        fprintf(stderr, "vak_udp_new failed\n");
        goto out;
    }

    impl = vak_impl_new(servers, 10, udp);
    if (!udp) {
        fprintf(stderr, "vak_impl_new failed\n");
        goto out;
    }

    while (1) {
        r = vak_impl_process(impl, plo, phi);
        if (r)
            break;
    }

out:
    if (impl)
        vak_impl_del(impl);
    if (udp)
        vak_udp_del(udp);
    if (servers)
        vak_servers_del(servers);

    return r;
}
