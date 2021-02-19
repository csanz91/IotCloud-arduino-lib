#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "iotcloud_constants.h"

const char IotCloud_Constants::AP_SSID_PREFIX[] = "iot_";
const char IotCloud_Constants::AP_PASSWORD[] = "12345678";
const char IotCloud_Constants::MQTT_SERVER[] = "iotcloud.es";
const char IotCloud_Constants::OTA_SERVER[] = "ota.iotcloud.es";
const char IotCloud_Constants::NTP_SERVER[] = "pool.ntp.org";
char IotCloud_Constants::AP_SSID[9] = "";
char IotCloud_Constants::DEVICE_INTERNAL_ID[4] = "";
bool IotCloud_Constants::ENABLE_OTA = true;

void IotCloud_Constants::setup_constants()
{
    char buffer[6];
    // Set up device ID
    itoa(ESP.getChipId(), buffer, 6);
    sprintf(IotCloud_Constants::DEVICE_INTERNAL_ID, "%.*s", 4, buffer + 2);

    // Set up
    IotCloud_Constants::AP_SSID[0] = '\0';
    strcat(IotCloud_Constants::AP_SSID, IotCloud_Constants::AP_SSID_PREFIX);
    strcat(IotCloud_Constants::AP_SSID, IotCloud_Constants::DEVICE_INTERNAL_ID);
}
