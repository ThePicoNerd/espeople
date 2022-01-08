#include <ESP8266WiFi.h>
#include "sdk_structs.h"
#include "iee80211_structs.h"
#include "credentials.h"
#include <Vector.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

InfluxDBClient influx_client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point pointMeasurement("measurement");

#define LED_BUILTIN 2

#define CHANNELS 13

// How many times to listen to each channel
#define CHANNEL_ROUNDS 4

// How long to sleep between measurements
#define SLEEP_MS 90e3

#define CHANNEL_HOP_INTERVAL_MS 1000

unsigned long measurement_start = 0;
Vector<uint64_t> addresses;

uint16_t packet_count = 0;
uint16_t legit_packet_count = 0;
uint16_t probe_requests = 0;

uint8_t rounds = 0;

void mac2str(const uint8_t *ptr, char *string)
{
  sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  return;
}

uint64_t mac_to_int(const uint8_t *ptr)
{
  return uint64_t(ptr[0]) << 40 |
         uint64_t(ptr[1]) << 32 | (
                                      // 32-bit instructions take fewer bytes on x86, so use them as much as possible.
                                      uint32_t(ptr[2]) << 24 | uint32_t(ptr[3]) << 16 | uint32_t(ptr[4]) << 8 | uint32_t(ptr[5]));
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

  if (frame_ctrl->type == WIFI_PKT_MGMT)
  {
    if (frame_ctrl->subtype == BEACON)
    {
      return; // beacos are dumb
    }

    if (frame_ctrl->subtype == PROBE_REQ)
    {
      probe_requests++;
    }
  }

  legit_packet_count++;

  uint64_t addr1 = mac_to_int(hdr->addr1);
  uint64_t addr2 = mac_to_int(hdr->addr2);
  // uint64_t addr3 = mac_to_int(hdr->addr3);

  // char a[] = "00:00:00:00:00:00\0";
  // char b[] = "00:00:00:00:00:00\0";

  // mac2str(hdr->addr1, a);
  // mac2str(hdr->addr2, b);

  // Serial.printf("%i,%s,%s,%i\n", frame_ctrl->type, frame_ctrl->subtype, a, b, ppkt->rx_ctrl.rssi);

  if (addresses.Size() == 0)
  {
    addresses.PushBack(addr1);
    addresses.PushBack(addr2);
    // addresses.PushBack(addr3);
  }
  else
  {
    for (int i = 0; i < addresses.Size(); i++)
    {
      if (addresses[i] == addr1)
        break;

      if (i == addresses.Size() - 1)
      {
        addresses.PushBack(addr1);
      }
    }

    for (int i = 0; i < addresses.Size(); i++)
    {
      if (addresses[i] == addr2)
        break;

      if (i == addresses.Size() - 1)
      {
        addresses.PushBack(addr2);
      }
    }

    // for (int i = 0; i < addresses.Size(); i++)
    // {
    //   if (addresses[i] == addr3)
    //     break;

    //   if (i == addresses.Size() - 1)
    //   {
    //     addresses.PushBack(addr3);
    //   }
    // }
  }
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

#define MAX_WIFI_DOTS 120

void flush_remote()
{
  unsigned long measure_millis = millis() - measurement_start;
  uint8_t wifi_count = 0;

  wifi_promiscuous_enable(0);
  wifi_set_opmode(WIFI_OFF);
  wifi_set_opmode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("connecting to %s ", WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    Serial.print(".");
    wifi_count++;

    if (wifi_count > MAX_WIFI_DOTS)
    {
      digitalWrite(LED_BUILTIN, HIGH);

      // cannot connect for 60 seconds
      Serial.println(" aborted!");
      ESP.restart();
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println(" connected!");

  Serial.print("waiting for ntp sync ");
  time_t now = time(nullptr);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  while (time(nullptr) < 1000000000l)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println(" done!");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("current time: ");
  Serial.print(asctime(&timeinfo));

  Serial.printf("found %i packets in %lims\n", packet_count, measure_millis);

  pointMeasurement.clearFields();

  float secs = float(measure_millis) / 1000.0;

  float pps = float(packet_count) / secs;
  float lpps = float(legit_packet_count) / secs;
  float prps = float(probe_requests) / secs;

  pointMeasurement.addField("pps", pps);
  pointMeasurement.addField("lpps", lpps);
  pointMeasurement.addField("prps", prps);
  pointMeasurement.addField("addresses", addresses.Size());

  Serial.print("Writing: ");
  Serial.println(pointMeasurement.toLineProtocol());

  if (!influx_client.writePoint(pointMeasurement))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(influx_client.getLastErrorMessage());
  }
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

  pointMeasurement.addTag("device", WiFi.macAddress());

  sniffer_init();

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
    WiFi.forceSleepBegin();
    delay(1);

    Serial.println("going to sleep, deep sleep not implemented :(");
    delay(SLEEP_MS);

    rounds = 0;
    packet_count = 0;
    legit_packet_count = 0;
    probe_requests = 0;
    addresses.Clear();

    WiFi.forceSleepWake();
    delay(1);

    sniffer_init();
    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
  }
}
