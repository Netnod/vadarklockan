#include "vak.h"

#include <stdlib.h>

#include <WiFiUdp.h>

struct vak_udp {
    WiFiUDP *udp;
};

struct vak_udp *vak_udp_new(void)
{
    struct vak_udp *udp =  new struct vak_udp;
    if (!udp)
        return NULL;

    memset(udp, 0, sizeof(*udp));

    udp->udp = new WiFiUDP();

    /* Create an UDP socket */
    udp->udp->begin(4711);

    return udp;
}

void vak_udp_del(struct vak_udp *udp)
{
    delete udp->udp;
    delete udp;
}

int vak_udp_send(struct vak_udp *udp, const char *host, unsigned port, const void *buffer, unsigned length)
{
    udp->udp->beginPacket(host, port);
    udp->udp->write((const uint8_t *)buffer, length);
    udp->udp->endPacket();

    return 0;
}

int vak_udp_recv(struct vak_udp *udp, void *buffer, unsigned length)
{
    int r = udp->udp->parsePacket();
    if (!r)
        return 0;

    return udp->udp->read((uint8_t *)buffer, length);
}
