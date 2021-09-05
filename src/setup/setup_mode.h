#ifndef _IOTCLOUD_SETUP_MODE
#define _IOTCLOUD_SETUP_MODE

#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>

#include "../iotcloud_constants.h"
#include "web_server.h"
#include "../device.h"

#define LED_PIN_PERIOD 1000U // milliseconds

extern void config_mode_setup(Device *device);
extern void config_mode_loop();

#endif // _IOTCLOUD_SETUP_MODE
