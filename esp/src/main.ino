#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "sdk_structs.h"

// const char *ssid = "ssid";
// const char *pass = "xxxxx";

#define LED_BUILTIN 2

#define CHANNELS 13

// How many times to listen to each channel
#define CHANNEL_ROUNDS 4

// How long to sleep between measurements (microseconds)
#define SLEEP_MS 10e3

#define CHANNEL_HOP_INTERVAL_MS 500

unsigned long measurement_start = 0;

uint16_t packet_count = 0;
uint8_t rounds = 0;

void mac2str(const uint8_t *ptr, char *string)
{
  sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  return;
}

void handle_packet(uint8_t *buf, uint16_t len)
{
  packet_count++;

  // First layer: type cast the received buffer into our generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  // Second layer: define pointer to where the actual 802.11 packet is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  // Third layer: define pointers to the 802.11 packet header and payload
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;

  // Pointer to the frame control section within the packet header
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  // char addr1[] = "00:00:00:00:00:00\0";
  // char addr2[] = "00:00:00:00:00:00\0";
  // char addr3[] = "00:00:00:00:00:00\0";

  // mac2str(hdr->addr1, addr1);
  // mac2str(hdr->addr2, addr2);
  // mac2str(hdr->addr3, addr3);

  // Output info to serial
  // Serial.printf("%s  %s  %s  %i\n", addr1, addr2, addr3, ppkt->rx_ctrl.rssi);
}

void sniffer_init()
{
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  wifi_set_promiscuous_rx_cb(handle_packet);
  wifi_promiscuous_enable(1);
  wifi_set_channel(1);

  measurement_start = millis();
}

void channel_hop()
{
  uint8_t new_channel = wifi_get_channel() + 1;
  if (new_channel > CHANNELS)
  {
    new_channel = 1;
    rounds++;
  }

  if (rounds < CHANNEL_ROUNDS)
  {
    Serial.printf("going to ch %i\n", new_channel);
    wifi_set_channel(new_channel);
  }
}

static os_timer_t channelHop_timer;

void flush_remote()
{
  unsigned long measure_millis = millis() - measurement_start;
  wifi_promiscuous_enable(0);
  wifi_set_opmode(WIFI_OFF);
  wifi_set_opmode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.printf("connecting to %s ", ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(450);
    Serial.print(".");
  }

  Serial.println(" connected!");

  Serial.printf("found %i packets in %ims\n", packet_count, measure_millis);

  WiFiClient wifi;

  HTTPClient http;

  http.begin(wifi, "http://192.168.0.38:8000/insert");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<32> doc;

  doc["packet_count"] = packet_count;
  doc["duration"] = measure_millis;

  String json;

  serializeJson(doc, json);

  int code = http.POST(json);

  Serial.printf("got code %i\n", code);

  http.end();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  digitalWrite(LED_BUILTIN, LOW); // LOW means on
  delay(1000);
  while (!Serial)
  {
  }
  digitalWrite(LED_BUILTIN, HIGH);

  sniffer_init();

  // Serial.printf("Connecting to %s ", ssid);
  // WiFi.begin(ssid, pass);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   digitalWrite(LED_BUILTIN, LOW);
  //   delay(50);
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   delay(450);
  //   Serial.print(".");
  // }

  // Serial.println(" connected");

  os_timer_disarm(&channelHop_timer);
  os_timer_setfn(&channelHop_timer, (os_timer_func_t *)channel_hop, NULL);
  os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}

void loop()
{
  if (rounds >= CHANNEL_ROUNDS)
  {
    Serial.printf("found %i packets\n", packet_count);
    os_timer_disarm(&channelHop_timer);

    flush_remote();

    wifi_set_opmode(WIFI_OFF);

    Serial.println("going to sleep, deep sleep not implemented :(");
    delay(SLEEP_MS);

    rounds = 0;
    packet_count = 0;

    sniffer_init();
    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
  }
}
