#ifndef _IOTCLOUD_OTA
#define _IOTCLOUD_OTA
#ifdef ESP8266
#include "ESP8266httpUpdate.h"
#elif defined(ARDUINO_ARCH_ESP32)
#include <HTTPUpdate.h>
#endif

#include "EspMQTTClient.h"

#include "IotCloud_config.h"
#include "iotcloud_constants.h"

static const char IOTCLOUD_OTA_ROOT_CA[] PROGMEM =
#include "certs/iotcloud_ota_pem.h"

    namespace OTA
{
    void _start_update();
    void _onUpdateRequest(const String &payload);
    bool _parseVersion(const char *version, unsigned long parts[3]);
    int _compareToCurrent(const char *requested);
    void _publishOtaStatus(int status, const char *info = nullptr);

    void init(const char *mqtt_ota_header, const char *current_version, const char *device_model, EspMQTTClient *mqtt_client);
    void loop();

#ifdef ARDUINO_ARCH_ESP32
    // Optional extra condition before a new OTA image is confirmed, on top of
    // MQTT staying connected. Use a function or a captureless lambda:
    //   OTA::set_health_check([] { return scanner.healthy_for(60000); });
    void set_health_check(bool (*check)());
    void _check_new_image();
#endif
} // namespace OTA

#endif // _IOTCLOUD_OTA