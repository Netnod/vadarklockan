/*
 *  This sketch sends random data over UDP on a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <time.h>

#include "login.h"

#include "vak.h"
#include "vrt.h"

int getentropy(void *ptr, size_t size)
{
    memset(ptr, 0x42, size);
    return 0;
}

static void print_time(uint64_t t64)
{
    struct tm *tm;
    unsigned hh, mm, ss;

    time_t t = t64 / 1000000;
    unsigned frac = t64 % 1000000;

    tm = gmtime(&t);

    hh = tm->tm_hour;
    mm = tm->tm_min;
    ss = tm->tm_sec;

    printf("%02u:%02u:%02u.%06u\n", hh, mm, ss, frac);
}

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

    if (settimeofday(&tv, NULL) < 0) {
        // TODO
    }

    print_time(t64);
}

/* How long to wait for a successful response to a roughtime query */
static const uint64_t QUERY_TIMEOUT_USECS = 1000000;

int vak_query_server(struct vak_server *server,
                     overlap_value_t *lo, overlap_value_t *hi)
{
    static uint32_t recv_buffer[VRT_QUERY_PACKET_LEN / 4] = {0};
    static uint8_t query_buf[VRT_QUERY_PACKET_LEN] = {0};
    uint32_t query_buf_len;
    WiFiUDP udp;
    uint64_t st, rt;
    static uint8_t nonce[VRT_NONCE_SIZE];
    uint64_t out_midpoint;
    uint32_t out_radii;

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

// WiFi network name and password:
static const char * networkName = username;
static const char * networkPswd = password;

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

void setup(){
    // Initilize hardware serial:
    Serial.begin(115200);

    //Connect to the WiFi network
    connectToWiFi(networkName, networkPswd);
}

void loop() {
    static uint64_t vak_done = 0;
    static uint64_t last_t64 = 0;

    if (!vak_done) {
        printf("waiting for WiFi...\n");
        if (connected) {
            overlap_value_t lo, hi;

            if (vak_main(&lo, &hi)) {
                double adj = (lo + hi) / 2;
                adjust_time(adj);
            }
            vak_done = 1;
        }
    }

    uint64_t t64 = get_time();

    if (t64 < last_t64 || t64 >= last_t64 + 1000000) {
        print_time(t64);
        last_t64 = t64;
    }
}
