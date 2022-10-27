#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vrt.h"
#include "vak.h"
#include "overlap_algo.h"

struct vak_impl {
    struct vak_server const * const * servers;
    unsigned wanted;
    struct vak_udp *udp;

    struct overlap_algo *algo;
    unsigned nr_servers;
    unsigned current_server;
    unsigned nr_queries;
    unsigned nr_responses;
    vak_time_t query_time;

    uint8_t buffer[VRT_QUERY_PACKET_LEN];
    uint8_t nonce[VRT_NONCE_SIZE];
};

struct vak_impl *vak_impl_new(struct vak_server const * const *servers, unsigned wanted, struct vak_udp *udp)
{
    struct vak_impl *impl;

    impl = malloc(sizeof(*impl));
    if (!impl)
        return NULL;

    memset(impl, 0, sizeof(*impl));

    impl->servers = servers;
    impl->wanted = wanted;
    impl->udp = udp;

    /* Create the overlap algorithm */
    impl->algo = overlap_new();
    if (!impl->algo) {
        fprintf(stderr, "overlap_new failed\n");
        vak_impl_del(impl);
        return 0;
    }

    /* count number of servers */
    for (impl->nr_servers = 0; impl->servers[impl->nr_servers]; impl->nr_servers++)
        ;
    impl->current_server = 0;

    impl->nr_queries = 0;
    impl->nr_responses = 0;

    return impl;
}

void vak_impl_del(struct vak_impl *impl)
{
    if (impl->algo)
        overlap_del(impl->algo);
    free(impl);
}

int vak_process(struct vak_impl *impl)
{
    struct vak_server const *server = impl->servers[impl->current_server];

    if (impl->nr_queries) {
        vak_time_t recv_time;

        int n;

        n = vak_udp_recv(impl->udp, impl->buffer, sizeof(impl->buffer));

        recv_time = vak_get_time();

        if (n <= 0) {
            // handle timeout
        } else {
            uint64_t midpoint;
            uint32_t radii;

            /* TODO We might want to verify that servaddr and respaddr
             * match before parsing the response.  This way we could
             * discard faked packets without having to verify the
             * signature.  */

            /* Verify the response, check the signature and that it
             * matches the nonce we put in the query. */
            if (vrt_parse_response(impl->nonce, 64, (void *)impl->buffer,
                                   n, server->public_key,
                                   &midpoint, &radii,
                                   server->variant) == VRT_SUCCESS) {
                fprintf(stderr, "vrt_parse_response failed\n");
            } else {
            }
        }

    }
}
