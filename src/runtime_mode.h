#ifndef _IOTCLOUD_RUNTIME_MODE
#define _IOTCLOUD_RUNTIME_MODE

#include <Arduino.h>
#include <TZ.h>
#include <time.h>
#include "EspMQTTClient.h"

#include "device_data.h"
#include "device.h"
#include "iotcloud_constants.h"
#include "ota.h"

extern void runtime_mode_setup(const device_data *data, Device *d);
extern void runtime_mode_loop();
void wifi_connect();

#endif // _IOTCLOUD_RUNTIME_MODE
