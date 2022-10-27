#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "vrt.h"
#include "vak.h"
#include "overlap_algo.h"

#if 0
static void hd(const void *buf, unsigned length)
{
    const uint8_t *ptr = buf;
    unsigned i;
    for (i = 0; i < length; i++) {
        printf(" %02x", ptr[i]);
    }
    printf("\n");
}
#endif

/* Number of overlapping responses required to succeed */
static const int WANTED_OVERLAPS = 3;

/* Maximum uncertainty (seconds) of overlap required to succeed */
static const double WANTED_UNCERTAINTY = 2.0;

/* How long to wait for a successful response to a roughtime query */
static const uint64_t QUERY_TIMEOUT_USECS = 1000000;

struct vak_impl {
    struct vak_server const **servers;
    unsigned wanted;
    struct vak_udp *udp;

    struct overlap_algo *algo;
    unsigned nr_servers;
    unsigned current_server;
    unsigned nr_queries;
    unsigned nr_responses;
    vak_time_t send_time;

    unsigned buffer_size;
    uint8_t *buffer;
    unsigned nonce_size;
    uint8_t *nonce;
};

struct vak_impl *vak_impl_new(struct vak_server const **servers, unsigned wanted, struct vak_udp *udp)
{
    struct vak_impl *impl;

    impl = malloc(sizeof(*impl));
    if (!impl)
        return NULL;

    memset(impl, 0, sizeof(*impl));

    impl->servers = servers;
    impl->wanted = wanted;
    impl->udp = udp;

    impl->buffer_size = VRT_QUERY_PACKET_LEN;
    impl->buffer = malloc(impl->buffer_size);
    if (!impl->buffer) {
        fprintf(stderr, "malloc buffer failed\n");
        vak_impl_del(impl);
        return NULL;
    }

    impl->nonce_size = VRT_NONCE_SIZE;
    impl->nonce = malloc(impl->nonce_size);
    if (!impl->nonce) {
        fprintf(stderr, "malloc nonce failed\n");
        vak_impl_del(impl);
        return NULL;
    }

    /* Create the overlap algorithm */
    impl->algo = overlap_new();
    if (!impl->algo) {
        fprintf(stderr, "overlap_new failed\n");
        vak_impl_del(impl);
        return NULL;
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
    free(impl->buffer);
    free(impl->nonce);
    if (impl->algo)
        overlap_del(impl->algo);
    free(impl);
}

static int vak_send_query(struct vak_impl *impl, const struct vak_server *server)
{
    int length;

    /* Create a random nonce.  This should be as good randomness as
     * possible, preferably cryptographically secure randomness. */
    if (getentropy(impl->nonce, impl->nonce_size) < 0) {
        fprintf(stderr, "getentropy(%u) failed: %s\n", (unsigned)impl->nonce_size, strerror(errno));
        return -1;
    }

    /* Fill in the query. */
    length = vrt_make_query(impl->buffer, impl->buffer_size,
                            impl->nonce, impl->nonce_size,
                            server->variant);
    if (length < 0) {
        fprintf(stderr, "vrt_make_query failed\n");
        return -1;
    }

    printf("%s:%u: send variant %u size %u\n", server->host, server->port, server->variant, length);
    fflush(stdout);

    impl->send_time = vak_get_time();

    if (vak_udp_send(impl->udp, server->host, server->port, impl->buffer, length) < 0) {
        fprintf(stderr, "vrt_udp_send failed\n");
        return -1;
    }

    return 0;
}

/* returns -1 on error (i.e. timeout), 1 if a good response was received, 0 if we need to try again */
static int vak_process_response(struct vak_impl *impl, const struct vak_server *server,
                                overlap_value_t *plo, overlap_value_t *phi)
{
    vak_time_t recv_time;

    int n;

    n = vak_udp_recv(impl->udp, impl->buffer, impl->buffer_size);

    recv_time = vak_get_time();

    if (n <= 0) {
        if (recv_time - impl->send_time > QUERY_TIMEOUT_USECS) {
            printf("timeout\n");
            return -1;
        }
    } else {
        uint64_t server_midp;
        uint32_t server_radi;

        printf("%s:%u: recv variant %u size %u\n", server->host, server->port, server->variant, n);
        fflush(stdout);

        /* TODO We might want to verify that servaddr and respaddr
         * match before parsing the response.  This way we could
         * discard faked packets without having to verify the
         * signature.  */

        /* Verify the response, check the signature and that it
         * matches the nonce we put in the query. */
        if (vrt_parse_response(impl->nonce, VRT_NONCE_SIZE, (void *)impl->buffer,
                                n, server->public_key,
                                &server_midp, &server_radi,
                                server->variant) == VRT_SUCCESS) {
            printf("midp %llu, radi %llu\n",
                   (unsigned long long)server_midp, (unsigned long long)server_radi);
            fflush(stdout);

            /* Translate roughtime response to lo..hi adjustment range.  */
            vak_time_t local_midp = (impl->send_time + recv_time) / 2;
            double local_rtt = (double)(recv_time - impl->send_time) / 1000000;
            double adjustment = ((double)server_midp - (double)local_midp) / 1000000;
            double uncertainty = (double)server_radi / 1000000 + local_rtt / 2;

            printf("adjustment %.3f, uncertainty %.3f, rtt %lld\n",
                   adjustment, uncertainty, (long long)local_rtt);

            *plo = adjustment - uncertainty;
            *phi = adjustment + uncertainty;

            printf("adj %.6f .. %.6f\n", *plo, *phi);

            return 1;
        }
    }

    return 0;
}

/* returns 1 if we have good time, 0 if we have not gotten time 1, -1 if we have no more servers to try */
int vak_impl_process(struct vak_impl *impl, overlap_value_t *plo, overlap_value_t *phi)
{
    /* if we have a query in flight, poll for a response */
    if (impl->nr_queries) {
        overlap_value_t lo, hi;

        struct vak_server const *server = impl->servers[impl->current_server];
        int r;

        r = vak_process_response(impl, server, &lo, &hi);
        if (r) {
            if (r > 0) {
                int nr_overlaps;

                impl->nr_responses++;

                overlap_add(impl->algo, lo, hi);
                nr_overlaps = overlap_find(impl->algo, &lo, &hi);

                printf("responses %u, overlaps %u, %.3f .. %.3f\n",
                       impl->nr_responses, nr_overlaps, lo, hi);

                if (nr_overlaps > impl->nr_responses / 2 &&
                    nr_overlaps >= WANTED_OVERLAPS &&
                    (hi - lo) <= WANTED_UNCERTAINTY) {

                    *plo = lo;
                    *phi = hi;

                    return 1;
                }
            }

            /* next server */
            impl->nr_queries--;
            impl->current_server++;
        }
    }

    /* if we have no query in flight, send out a new one */
    while (!impl->nr_queries) {
        struct vak_server const *server = impl->servers[impl->current_server];

        if (!server) {
            fprintf(stderr, "no more servers\n");
            return -1;
        }

        if (vak_send_query(impl, server) < 0) {
            impl->current_server++;
            continue;
        }

        impl->nr_queries++;
    }

    return 0;
}
