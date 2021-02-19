#ifndef _IOTCLOUD
#define _IOTCLOUD

#include "device_data.h"
#include "setup/setup_mode.h"
#include "runtime_mode.h"
#include "iotcloud_constants.h"
#include "device.h"

extern void iotcloud_setup(Device *device);
extern void iotcloud_loop();
void detect_reset_request();

#endif // _IOTCLOUD