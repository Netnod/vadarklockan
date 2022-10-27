#include "vak.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

struct vak_udp {
    int sockfd;
};

struct vak_udp *vak_udp_new(void)
{
    struct vak_udp *udp = malloc(sizeof(*udp));
    if (!udp)
        return NULL;

    memset(udp, 0, sizeof(*udp));

    /* Create an UDP socket */
    udp->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp->sockfd < 0) {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        vak_udp_del(udp);
        return NULL;
    }

    return udp;
}

void vak_udp_del(struct vak_udp *udp)
{
    if (udp->sockfd != -1)
        close(udp->sockfd);
    free(udp);
}

int vak_udp_send(struct vak_udp *udp, const char *host, unsigned port, const void *buffer, unsigned length)
{
    struct hostent *he;
    struct sockaddr_in addr;
    int n;

    /* Look up the host name or process the IPv4 address and fill in
     * the addr structure. */
    he = gethostbyname(host);
    if (!he) {
        fprintf(stderr, "gethostbyname failed: %s\n", strerror(errno));
        return -1;
    }

    // TODO support IPv6

    memset(&addr, 0, sizeof(addr));
    memcpy(&addr.sin_addr.s_addr, he->h_addr_list[0], sizeof(struct in_addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    /* Send the request */
    n = sendto(udp->sockfd, buffer, length, 0,
               (const struct sockaddr *)&addr, sizeof(addr));
    if (n != length) {
        fprintf(stderr, "sendto failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int vak_udp_recv(struct vak_udp *udp, void *buffer, unsigned length)
{
    fd_set readfds;
    struct timeval tv;
    int n;
    struct sockaddr_in addr;
    socklen_t addrsize = sizeof(addr);

    FD_ZERO(&readfds);
    FD_SET(udp->sockfd, &readfds);

    /* Use a tenth of the query timeout for the select call. */
    tv.tv_sec = 0;
    tv.tv_usec = 100000;        /* 0.1s */

    n = select(udp->sockfd + 1, &readfds, NULL, NULL, &tv);
    if (n == -1) {
        if (errno == EINTR)
            return 0;
        fprintf(stderr, "select failed: %s\n", strerror(errno));
        return -1;
    }

    if (!FD_ISSET(udp->sockfd, &readfds))
        return 0;

    n = recvfrom(udp->sockfd, buffer, length,
                 MSG_DONTWAIT /* flags */,
                 (struct sockaddr *)&addr, &addrsize);
    if (n < 0) {
        if (errno == EAGAIN)
            return 0;
        fprintf(stderr, "recv failed: %s\n", strerror(errno));
        return -1;
    }

    return n;
}

