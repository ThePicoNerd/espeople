#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "./esppl.h"

const char *ssid = "";
const char *pass = "";

#define LED_BUILTIN 2

using namespace std;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  digitalWrite(LED_BUILTIN, LOW); // LOW means on
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);

  esppl_init(cb);
  esppl_sniffing_start();

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
}

void cb(esppl_frame_info *info)
{
  Serial.printf("\n  %02X:%02X:%02X:%02X:%02X:%02X", info->transmitteraddr[0], info->transmitteraddr[1],
                info->transmitteraddr[2], info->transmitteraddr[3], info->transmitteraddr[4], info->transmitteraddr[5]);
                Serial.printf(" >> %02X:%02X:%02X:%02X:%02X:%02X", info->receiveraddr[0], info->receiveraddr[1],
                info->receiveraddr[2], info->receiveraddr[3], info->receiveraddr[4], info->receiveraddr[5]);
}

void loop()
{
  for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++)
  {
    esppl_set_channel(i);
    while (esppl_process_frames())
    {
      // Serial.print(".");
    }
  }
}
