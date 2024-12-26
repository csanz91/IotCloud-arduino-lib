#include <Arduino.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif

#include "iotcloud_constants.h"

const char IotCloud_Constants::AP_SSID_PREFIX[] = "iot";
const char IotCloud_Constants::AP_PASSWORD[] = "12345678";
const char IotCloud_Constants::MQTT_SERVER[] = "mqtt.iotcloud.es";
const char IotCloud_Constants::OTA_SERVER[] = "ota.iotcloud.es";
const char IotCloud_Constants::NTP_SERVER[] = "pool.ntp.org";
char IotCloud_Constants::AP_SSID[9] = "";
char IotCloud_Constants::DEVICE_INTERNAL_ID[14] = "";
bool IotCloud_Constants::ENABLE_OTA = true;

void IotCloud_Constants::setup_constants()
{
    byte mac[6];
    WiFi.macAddress(mac);
    // Set up device ID
    sprintf(IotCloud_Constants::DEVICE_INTERNAL_ID, "%02x%02x%02x%02x-%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Set up config mode Wifi SSID
    sprintf(IotCloud_Constants::AP_SSID, "%s_%02x%02x", IotCloud_Constants::AP_SSID_PREFIX, mac[4], mac[5]);
}
