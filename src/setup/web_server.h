#ifndef _IOTCLOUD_WEB_SERVER
#define _IOTCLOUD_WEB_SERVER

#include <ArduinoJson.h>
#include "../device_data.h"
#include "../device.h"

extern void web_server_setup(Device *d);
extern void web_server_loop();

#endif // _IOTCLOUD_WEB_SERVER
