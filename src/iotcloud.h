#ifndef _IOTCLOUD
#define _IOTCLOUD

#include "device_data.h"
#include "setup/setup_mode.h"
#include "runtime_mode.h"
#include "iotcloud_constants.h"
#include "IotCloud_config.h"
#include "device.h"

#ifndef IOTCLOUD_RESET_PIN
  #ifdef CONFIG_IDF_TARGET_ESP32C3
    #define IOTCLOUD_RESET_PIN 9
  #else
    #define IOTCLOUD_RESET_PIN 0
  #endif
#endif
#define RESET_PRESS_TIME 5000 // milliseconds

extern void iotcloud_setup(Device *device, unsigned long offline_restart_timeout_ms = 0);
extern void iotcloud_loop();
void detect_reset_request();

#endif // _IOTCLOUD
