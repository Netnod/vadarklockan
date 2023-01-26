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

// Graphics and font library for ST7735 driver chip
#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// WiFi network name and password:
static const char *networkName = WIFI_USERNAME;
static const char *networkPswd = WIFI_PASSWORD;

//Are we currently connected?
static boolean connected = false;

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

    time_t t = vt / 1000000;
    unsigned frac = vt % 1000000;

    tm = gmtime(&t);

    printf("%04u-%02u-%02u %02u:%02u:%02u.%06u\n",
           tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
           tm->tm_hour, tm->tm_min, tm->tm_sec, frac);

    char sdate[11], shh[3], smm[3], sss[3];
    snprintf(sdate, sizeof(sdate), "%04u-%02u-%02u",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    snprintf(shh, sizeof(shh), "%02u", tm->tm_hour);
    snprintf(smm, sizeof(smm), "%02u", tm->tm_min);
    snprintf(sss, sizeof(sss), "%02u", tm->tm_sec);

    const uint16_t color1 = 0xfbe0;     // orange
    const uint16_t color2 = 0x39c4;     // gray
    const unsigned time_font = 7;            // LCD-like font

    // flash colon
    uint16_t colon_color = color1;
    if (tm->tm_sec & 1)
        colon_color = color2;

    byte xpos = 7;
    byte ypos = 20;

    tft.setTextColor(color1, TFT_BLACK);
    xpos += tft.drawString(shh, xpos, ypos, time_font);

    tft.setTextColor(colon_color, TFT_BLACK);
    xpos += tft.drawChar(':', xpos, ypos, time_font);

    tft.setTextColor(color1, TFT_BLACK);
    xpos += tft.drawString(smm, xpos, ypos, time_font);

    tft.setTextColor(colon_color, TFT_BLACK);
    xpos += tft.drawChar(':', xpos, ypos, time_font);

    tft.setTextColor(color1, TFT_BLACK);
    xpos += tft.drawString(sss, xpos, ypos, time_font);

    ypos += 60;

    tft.setTextColor(color1, TFT_BLACK);
    tft.drawString(sdate, 7, ypos, 4);
    tft.drawString("UTC", 170, ypos, 4);

    ypos += 30;
    xpos = 60;
    if (!connected)
        tft.drawString("   No WiFi             ", xpos, ypos, 4);
    else
        tft.drawString("Connected", xpos, ypos, 4);
}

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

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

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
