#ifndef _IOTCLOUD_DEVICE
#define _IOTCLOUD_DEVICE

#ifdef ESP8266
    #include "ESP8266httpUpdate.h"
#elif defined(ESP32)
    #include <HTTPUpdate.h>
#endif

#include "EspMQTTClient.h"

#include "sensors/sensor.h"
#include "device_data.h"
#include "iotcloud_constants.h"

class Device
{
private:
    const char *_deviceInternalId;
    std::vector<BaseSensor *> _sensors;

public:
    Device(
        const char *device_current_version,
        const char *device_model);
    ~Device();

    char mqtt_header[70];
    char mqtt_status_topic[76];
    const char *device_current_version;
    const char *device_model;

    inline void add_sensor(BaseSensor *sensor) { _sensors.push_back(sensor); };
    void init(EspMQTTClient *mqtt_client);
    void export_data(char *exported_data);
    void get_topic(char *topic, const char *endpoint);
    void loop();
};

#endif // _IOTCLOUD_DEVICE