/*
 *  This sketch sends random data over UDP on a ESP32 device
 *
 */

#include "vak.h"
#include "vrt.h"
#include "login.h"

#include <time.h>
#include <unistd.h>

#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

/* ESP does not have this function.  It's present in the include files
 * but there's no implementation.  Make it a wrapper around a similar
 * ESP function */
int getentropy(void *buffer, size_t length)
{
    esp_fill_random(buffer, length);
    return 0;
}

static void print_time(vak_time_t vt)
{
    struct tm *tm;
    unsigned hh, mm, ss;

    time_t t = vt / 1000000;
    unsigned frac = vt % 1000000;

    tm = gmtime(&t);

    hh = tm->tm_hour;
    mm = tm->tm_min;
    ss = tm->tm_sec;

    printf("%02u:%02u:%02u.%06u\n", hh, mm, ss, frac);
}

#if 0
/* How long to wait for a successful response to a roughtime query */
static const uint64_t QUERY_TIMEOUT_USECS = 1000000;

int vak_query_server(struct vak_server *server,
                     overlap_value_t *lo, overlap_value_t *hi)
{
    /* Note that the default stack on ESP32 is too small for these
     * buffers to be allocated on the stack, so they are static.  It
     * would probably be better to allocate them using malloc/free. */

    static uint8_t query_buf[VRT_QUERY_PACKET_LEN] = {0};
    static uint32_t recv_buffer[VRT_QUERY_PACKET_LEN / 4] = {0};
    static uint8_t nonce[VRT_NONCE_SIZE];

    uint32_t query_buf_len;
    uint64_t st, rt;
    uint64_t out_midpoint;
    uint32_t out_radii;

    WiFiUDP udp;

    /* Create an UDP socket */
    udp.begin(WiFi.localIP(), server->port);
    udp.beginPacket(server->host, server->port);

    /* Get the wall time just before the request was sent out. */
    st = get_time();

    udp.write(query_buf, query_buf_len);
    udp.endPacket();

    /* Keep waiting until we get a valid response or we time out */
    while (1) {
        int r = udp.parsePacket();

        /* Get the time as soon after the packet was received. */
        rt = get_time();

        if (r) {
            unsigned n = udp.read((char*)recv_buffer, sizeof(recv_buffer));

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
            return 0;
        }
    }

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
#endif

// WiFi network name and password:
static const char *networkName = WIFI_USERNAME;
static const char *networkPswd = WIFI_PASSWORD;

//Are we currently connected?
static boolean connected = false;

static void connectToWiFi(const char *ssid, const char *pwd){
    Serial.println("Connecting to WiFi network: " + String(ssid));

    // delete old config
    WiFi.disconnect(true);
    //register event handler
    WiFi.onEvent(WiFiEvent);

    //Initiate connection
    WiFi.begin(ssid, pwd);

    Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          //When connected set
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());
          //initializes the UDP state
          //This initializes the transfer buffer

          connected = true;
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}

static enum {
    VAK_NEW,
    VAK_RUNNING,
    VAK_DONE,
} vak_state = VAK_NEW;
static struct vak_server const **vak_servers;
static struct vak_udp *vak_udp;
static struct vak_impl *vak_impl;

void setup(){
    // Initilize hardware serial:
    Serial.begin(115200);

    //Connect to the WiFi network
    connectToWiFi(networkName, networkPswd);

    if (vak_seed_random() < 0) {
        fprintf(stderr, "vak_seed_random failed: %s\n", strerror(errno));
        // TODO we should not even start using vak
    } else if (!(vak_servers = vak_get_randomized_servers())) {
        fprintf(stderr, "vak_get_randomized_servers failed\n");
    } else if (!(vak_udp = vak_udp_new())) {
        fprintf(stderr, "vak_udp_new failed\n");
    } else if (!(vak_impl = vak_impl_new(vak_servers, 10, vak_udp))) {
        fprintf(stderr, "vak_impl_new failed\n");
    } else {
        vak_state = VAK_RUNNING;
    }
}

void loop() {
    static uint64_t last_seconds = 0;

    vak_time_t vt = vak_get_time();
    uint64_t seconds = vt / 1000000;
    if (last_seconds != seconds) {
        print_time(vt);
        last_seconds = seconds;

        if (!connected)
            printf("waiting for WiFi...\n");
    }

    if (vak_state == VAK_RUNNING) {
        if (connected) {
            overlap_value_t lo, hi;

            int r = vak_impl_process(vak_impl, &lo, &hi);
            if (r) {
                if (r < 0) {
                    fprintf(stderr, "VAK failed, giving up on getting time\n");
                } else {
                    // use midpoint as adjustment
                    printf("VAK success adjustment range %.3f .. %.3f\n", lo, hi);
                    double adj = (lo + hi) / 2;
                    if (vak_adjust_time(adj * 1000000) < 0) {
                        fprintf(stderr, "vak_adjust_time failed: %s\n", strerror(errno));
                    }
                }

                vak_state = VAK_DONE;

                vak_impl_del(vak_impl);
                vak_udp_del(vak_udp);
                vak_servers_del(vak_servers);
            }
        }
    }
}
