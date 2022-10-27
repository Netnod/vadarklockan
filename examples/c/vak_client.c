#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "vak.h"
#include "vrt.h"

/** Get the wall time.
 *
 * This functions uses the same representation of time as the vrt
 * functions, i.e. the number of microseconds the epoch which is
 * midnight 1970-01-01.
 *
 * \returns The number of microseconds since the epoch.
 */
static uint64_t get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t t =
        (uint64_t)(tv.tv_sec) * 1000000 +
        (uint64_t)(tv.tv_usec);
    return t;
}

static void adjust_time(double adj)
{
    uint64_t t64;
    struct timeval tv;

    t64 = get_time();
    t64 += adj * 1000000;

    tv.tv_sec = t64 / 1000000;
    tv.tv_usec = t64 % 1000000;

    if (settimeofday(&tv, NULL) < 0)
        fprintf(stderr, "settimeofday failed: %s\n", strerror(errno));
}

/* How long to wait for a successful response to a roughtime query */
static const uint64_t QUERY_TIMEOUT_USECS = 1000000;

int vak_query_server(struct vak_server *server, overlap_value_t *lo, overlap_value_t *hi)
{
    static uint8_t query_buf[VRT_QUERY_PACKET_LEN] = {0};
    static uint32_t recv_buffer[VRT_QUERY_PACKET_LEN / 4] = {0};
    static uint8_t nonce[VRT_NONCE_SIZE];

    uint32_t query_buf_len;
    uint64_t st, rt;
    uint64_t out_midpoint;
    uint32_t out_radii;

    struct hostent *he;
    struct sockaddr_in servaddr;
    struct sockaddr_in respaddr;
    int sockfd;

    /* Create a random nonce.  This should be as good randomness as
     * possible, preferably cryptographically secure randomness. */
    if (getentropy(nonce, sizeof(nonce)) < 0) {
        fprintf(stderr, "getentropy(%u) failed: %s\n", (unsigned)sizeof(nonce), strerror(errno));
        return 0;
    }

    /* Fill in the query. */
    query_buf_len = sizeof(query_buf);
    if (vrt_make_query(nonce, 64, query_buf, &query_buf_len, server->variant) != VRT_SUCCESS) {
        fprintf(stderr, "vrt_make_query failed\n");
        return 0;
    }

    printf("%s:%u: variant %u size %u ", server->host, server->port, server->variant, (unsigned)query_buf_len);
    fflush(stdout);

    /* Look up the host name or process the IPv4 address and fill in
     * the servaddr structure. */
    he = gethostbyname(server->host);
    if (!he) {
        fprintf(stderr, "gethostbyname failed: %s\n", strerror(errno));
        return 0;
    }

    memset((char *)&servaddr, 0, sizeof(servaddr));
    memcpy(&servaddr.sin_addr.s_addr, he->h_addr_list[0], sizeof(struct in_addr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server->port);

    /* Create an UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        return 0;
    }

    /* Get the wall time just before the request was sent out. */
    st = get_time();

    /* Send the request */
    int n = sendto(sockfd, (const char *)query_buf, query_buf_len, 0,
                   (const struct sockaddr *)&servaddr, sizeof(servaddr));
    if (n != query_buf_len) {
        fprintf(stderr, "sendto failed: %s\n", strerror(errno));
        close(sockfd);
        return 0;
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    /* Keep waiting until we get a valid response or we time out */
    while (1) {
        fd_set readfds;
        int n;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        /* Use a tenth of the query timeout for the select call. */
        tv.tv_sec = 0;
        tv.tv_usec = QUERY_TIMEOUT_USECS / 10;

        n = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "select failed: %s\n", strerror(errno));
            close(sockfd);
            return 0;
        }

        /* Get the time as soon after the packet was received. */
        rt = get_time();

        /* Get the packet from the kernel. */
        if (FD_ISSET(sockfd, &readfds)) {
            socklen_t respaddrsize = sizeof(respaddr);
            n = recvfrom(sockfd, recv_buffer,
                         (sizeof recv_buffer) * sizeof recv_buffer[0],
                         0 /* flags */,
                         (struct sockaddr *)&respaddr, &respaddrsize);
            if (n < 0) {
                if (errno == EAGAIN)
                    continue;
                fprintf(stderr, "recv failed: %s\n", strerror(errno));
                close(sockfd);
                return 0;
            }

            /* TODO We might want to verify that servaddr and respaddr
             * match before parsing the response.  This way we could
             * discard faked packets without having to verify the
             * signature.  */

            /* Verify the response, check the signature and that it
             * matches the nonce we put in the query. */
            if (vrt_parse_response(nonce, 64, recv_buffer,
                                   n,
                                   server->public_key, &out_midpoint,
                                   &out_radii, server->variant) != VRT_SUCCESS) {
                fprintf(stderr, "vrt_parse_response failed\n");
                continue;
            }

            /* Break out of the loop. */
            break;
        }

        if (rt - st > QUERY_TIMEOUT_USECS) {
            printf("timeout\n");
            close(sockfd);
            return 0;
        }
    }

    close(sockfd);

    /* Translate roughtime response to lo..hi adjustment range.  */
    uint64_t local_time = (st + rt) / 2;
    double adjustment = ((double)out_midpoint - (double)local_time)/1000000;
    double rtt = (double)(rt - st) / 1000000;
    double uncertainty = (double)out_radii / 1000000 + rtt / 2;

    *lo = adjustment - uncertainty;
    *hi = adjustment + uncertainty;

    printf("adj %.6f .. %.6f\n", *lo, *hi);

    return 1;
}

int main(int argc, char *argv[]) {
    unsigned seed;
    overlap_value_t lo, hi;

    /* Seed the random function with some randomness.  This is used to
     * randomize the list of vak_servers. */
    if (getentropy(&seed, sizeof(seed)) < 0) {
        fprintf(stderr, "getentropy(%u) failed: %s\n", (unsigned)sizeof(seed), strerror(errno));
        return 0;
    }
    srandom(seed);

    // TODO add command line argument "-q" to only query
    // TODO add command line argument "-v" for verbose

    if (vak_main(&lo, &hi)) {
        /* If this code is enabled, adjust the local clock. */
        if (1) {
            double adj = (lo + hi) / 2;
            adjust_time(adj);
        }
    }

    exit(0);
}
