#ifndef VAK_H
#define VAK_H

#include <stdint.h>

#include "overlap_algo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t vak_time_t;

struct vak_impl;
struct vak_udp;

/** Structure describing a roughtime server */
struct vak_server {
    /** Hostname or IP address */
    const char *host;

    /** Port number */
    unsigned port;

    /** Protocol variant, the roughtime draft number */
    unsigned variant;

    /** Server's ed2551 public key */
    uint8_t public_key[32];
};

vak_time_t vak_get_time(void);

void vak_impl_del(struct vak_impl *impl);

struct vak_udp *vak_udp_new(void);
int vak_udp_send(struct vak_udp *udp, const char *host, unsigned port, const void *buffer, unsigned length);
int vak_udp_recv(struct vak_udp *udp, const void *buffer, unsigned length);
void vak_udp_del(struct vak_udp *udp);

struct vak_server **vak_get_servers(void);
struct vak_server **vak_get_randomized_servers(void);
void vak_del_servers(struct vak_server **servers);

int vak_main(overlap_value_t *plo, overlap_value_t *phi);

/* Query a roughtime server to find out how to adjust our local clock
 * to match the time from the server.
 *
 * The adjustment is a range (lo, hi) which takes into account the
 * server's reported uncertainty and the round trip time for the
 * query.
 *
 * \param server a rt_server structure describing the server to query
 * \param lo the low value of the range is written to this pointer
 * \param hi the high value of the range is written to this pointer
 * \returns 1 on success or 0 on failure
 *
 * This function contains some platform specific code.
 * Implementations for other platforms will look slightly different.
 * The concepts should be the same though.
 *
 * Set up an UDP socket.
 *
 * Create a random nonce.
 *
 * Fill in a buffer with a roughtime query by calling vrt_make_query.
 *
 * Save the time the query was sent.
 *
 * Send the query to the server and wait for a response.
 *
 * Save the time the response was recevied.
 *
 * Process the response using vrt_parse_response which also verifies
 * the signature and that the nonce matches.  If that succeeded,
 * return the lo and hi adjustments.
 *
 * If the processing failed, and the request has not timed out yet, go
 * back and wait for more responses.
 */
int vak_query_server(struct vak_server *server, overlap_value_t *lo, overlap_value_t *hi);

#ifdef __cplusplus
}
#endif

#endif /* VAK_H */
