/*
 *  This sketch sends random data over UDP on a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <time.h> 

#include "login.h"
extern "C" {
#include "clusteringAlgorithm.h"
#include "vrt.h"
#include "tweetnacl.h"
}

#define CHECK(x)                                                               \
  do {                                                                         \
    int ret;                                                                   \
    if ((ret = x) != VRT_SUCCESS) {                                            \
    fprintf(stderr, "%s:%u: ret %u\n", __func__, __LINE__, ret); \
      return (ret);                                                            \
    }                                                                          \
  } while (0)

struct rt_server {
    const char *host;
    unsigned port;
    int variant;
    uint8_t public_key[32];
};

struct rt_server servers[] = {
    { "209.50.50.228", 2002, 0, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 0, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 0, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 1, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "193.180.164.51", 2002, 1, { 0x19,0xd1,0xa9,0xf6,0x6e,0x40,0x6f,0x0a,0x82,0x3a,0x94,0xd5,0x62,0xaf,0xb2,0x96,0x20,0x48,0xaf,0x38,0x75,0xc1,0xe7,0x7b,0x55,0x84,0x52,0xe4,0x2c,0x4c,0x63,0x75 } }, /* roughtime.weinigel.se */
    { "194.58.207.198", 2002, 1, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 1, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 1, { 0xb4,0x03,0xec,0x41,0xcd,0xc3,0xdf,0xa9,0x89,0x3c,0xe5,0xf5,0xfc,0xb2,0xcd,0x6d,0x5d,0x0c,0xdd,0xfb,0x93,0x3e,0x3c,0x16,0xe7,0x89,0x86,0xbf,0x0f,0x95,0xd6,0x11 } }, /* lab.roughtime.netnod.se */
    { "194.58.207.197", 2002, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2000, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2001, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2003, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2004, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2005, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2006, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2007, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2008, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2009, 1, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
};

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

uint32_t targetTime = 0;       // for next 1 second timeout
byte omm = 99;
bool initial = 1;
byte xcolon1 = 0;
byte xcolon2 = 0;
byte colon_size = 0;
unsigned int colour = 0;
uint8_t hh, mm, ss;

// WiFi network name and password:
const char * networkName = username;
const char * networkPswd = password;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;

// Everything that has to do with VAK
clusteringData *server_cluster_data = NULL;
int needed_answers = 4;

void setup(){
  // Initilize hardware serial:
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  targetTime = millis() + 1000; 
  
  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);
}

void loop() {

  if (connected){
    for (int j = 0; j<1; j++){
    server_cluster_data = createClusterData(needed_answers);
    int i;
    for (i = 0; i < sizeof(servers) / sizeof(servers[0]); i++) {
        Serial.println( String(servers[i].host) + ":" + String(servers[i].port) + "\n");
        if(doit(&servers[i], server_cluster_data) == -1){
          Serial.println("Something went wrong in doit\n");
        }
     }

    double adj = get_adjustment(server_cluster_data);
    set_time(adj);
    
    // Free the allocated memory (may want to check if there are leaks)
    free_tree(server_cluster_data);
    }
    connected = false;
  }
  
  if (targetTime < millis()) {
    targetTime = millis()+1000;
    ss++;              // Advance second
    if (ss==60) {
      ss=0;
      omm = mm;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
        }
      }
    }

    // Update digital time
    byte xpos = 6;
    byte ypos = 3;
    if (omm != mm) { // Only redraw every minute to minimise flicker
      tft.setTextColor(TFT_BLACK, TFT_BLACK); // Set font colour to black to wipe image
      
      tft.drawString("88:88:88",xpos,ypos,7); // Overwrite the text to clear it
      tft.setTextColor(0xFBE0); // Orange
      omm = mm;

      if (hh<10) xpos+= tft.drawChar('0',xpos,ypos,7);
      xpos+= tft.drawNumber(hh,xpos,ypos,7);
      xcolon1=xpos;
      xpos+= tft.drawChar(':',xpos,ypos,7);
      if (mm<10) xpos+= tft.drawChar('0',xpos,ypos,7);
      xpos+= tft.drawNumber(mm,xpos,ypos,7);
      xcolon2=xpos;
      colon_size= tft.drawChar(':',xpos,ypos,7);
      xpos+= colon_size;
      if (ss<10) xpos+= tft.drawChar('0',xpos,ypos,7);
      tft.drawNumber(ss,xpos,ypos,7);
    }

    tft.fillRect(xcolon2+1, ypos, 100,100,TFT_BLACK);
    byte adjustment = colon_size;
    if (ss<10) adjustment+=tft.drawChar('0',xcolon2+colon_size,ypos,7);
    tft.drawNumber(ss,xcolon2+adjustment,ypos,7);

    if (ss%2) { // Flash the colon
      tft.setTextColor(0x39C4, TFT_BLACK);
      tft.drawChar(':',xcolon1,ypos,7);
      tft.drawChar(':',xcolon2,ypos,7);
      tft.setTextColor(0xFBE0, TFT_BLACK);
    }
    else {
      tft.drawChar(':',xcolon1,ypos,7);
      tft.drawChar(':',xcolon2,ypos,7);
    }
  }
}

void connectToWiFi(const char *ssid, const char *pwd){
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

// Uses a time.h function to get the time
uint64_t get_time()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t t = 
    (uint64_t)(tv.tv_sec) * 1000000 +
    (uint64_t)(tv.tv_usec);

  return t;
}

int set_time(double adj)
{
  time_t sec = (int)adj;
  time_t usec = (int)(adj - (int)adj)*10^6;
  
  struct timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = usec;
  settimeofday(&tv, NULL);

  set_clock();
}

void set_clock()
{
  uint64_t curr_time = get_time();
  
  time_t t;
  t = curr_time / 1000000;
  unsigned microseconds = curr_time % 1000000;
  struct tm *tm = gmtime(&t);

  hh = tm->tm_hour;
  mm = tm->tm_min;
  ss = tm->tm_sec;

  printf("%02u:%02u:%02u\n", hh, mm, ss);
}

int doit(struct rt_server *server, clusteringData *server_cluster_data){

  uint32_t recv_buffer[VRT_QUERY_PACKET_LEN / 4] = {0};
  uint8_t query[VRT_QUERY_PACKET_LEN] = {0};
  uint64_t out_midpoint;
  uint32_t out_radii;

  /* prepare query */
  uint8_t nonce[VRT_NONCE_SIZE] = "preferably a random byte buffer";
  CHECK(vrt_make_query(nonce, sizeof(nonce), query, sizeof(query), server->variant));
  
  /* send query */
  udp.begin(WiFi.localIP(), server->port);
  udp.beginPacket(server->host, server->port);

  // Gets the current time (us since epoch)
  uint64_t st = get_time();
  
  clock_t function_time, temp;
  function_time = clock();
  if(function_time == -1) return -1;
  udp.write(query, sizeof(query));
  udp.endPacket();
  while(udp.parsePacket() == 0){}

  // The time it took for the message to be sent and recieved
  temp = clock();
  if(temp == -1) return -1;
  function_time = temp - function_time;

  // Gets current time (us since epoch)
  uint64_t rt = get_time();

  // Calculates RTT
  double rtt = ((double)function_time)/CLOCKS_PER_SEC;
  
  unsigned n = udp.read((char*)recv_buffer, sizeof(query));
  
  if(n > 0)
  {
    CHECK(vrt_parse_response(nonce, sizeof(nonce), recv_buffer,
                            n,
                            server->public_key, &out_midpoint,
                            &out_radii, server->variant));

    double uncertainty = (double)out_radii/1000000 + rtt/2;
    
    uint64_t local = (st + rt) / 2;
    double adjustment = 0;
    uint64_t mjd_time;
    
    // Take into account the server version when deciding adjustment
    if(server->variant == 0){
      adjustment = ((double)out_midpoint - (double)local)/1000000;
  
    }else{
      unsigned mjd = out_midpoint >> 40;
      uint64_t microseconds_from_midnight = out_midpoint & 0xffffffffff;
      mjd_time = (mjd - 40587) * 86400000000 + microseconds_from_midnight;
      adjustment = ((double)mjd_time - (double)local)/1000000;
    }
  
    // Finds the overlap from the edges
    if(find_overlap(server_cluster_data, adjustment, uncertainty) == -1){
      return -1;
    }
  }
  return 0;
}
