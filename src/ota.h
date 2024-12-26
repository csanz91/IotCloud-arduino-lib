#ifndef _IOTCLOUD_OTA
#define _IOTCLOUD_OTA
#ifdef ESP8266
#include "ESP8266httpUpdate.h"
#elif defined(ESP32)
#include <HTTPUpdate.h>
#endif

#include "EspMQTTClient.h"

#include "iotcloud_constants.h"

static const char LETSENCRYPT_ROOT_CA[] PROGMEM =
#include "certs/iotcloud_ota_pem.h"

    namespace OTA
{
    void _start_update();
    void _publishOtaStatus(int status, const char *info = nullptr);

    void init(const char *mqtt_ota_header, const char *current_version, const char *device_model, EspMQTTClient *mqtt_client);
    void loop();
} // namespace OTA

#endif // _IOTCLOUD_OTA