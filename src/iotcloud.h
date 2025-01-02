#ifndef _IOTCLOUD
#define _IOTCLOUD

#include "device_data.h"
#include "setup/setup_mode.h"
#include "runtime_mode.h"
#include "iotcloud_constants.h"
#include "device.h"

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define RESET_PIN 9
#else
#define RESET_PIN 0
#endif
#define RESET_PRESS_TIME 5000 // milliseconds

extern void iotcloud_setup(Device *device);
extern void iotcloud_loop();
void detect_reset_request();

#endif // _IOTCLOUD