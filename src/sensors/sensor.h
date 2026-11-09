#ifndef _IOTCLOUD_BASE_SENSOR
#define _IOTCLOUD_BASE_SENSOR

#include <ArduinoJson.h>

#include "../IotCloud_config.h"
#include "EspMQTTClient.h"
#include "../iotcloud_constants.h"

class BaseSensor
{
private:
    const char *_sensor_id_suffix;
    char _sensor_id[64];
    const char *_sensor_type;
    const char *_sensor_sub_type;
    const char *_metadata;
    void refresh_sensor_id();

protected:
    char *_mqtt_header;
    EspMQTTClient *_mqtt_client = 0;
    const char *_sensor_name;
    const char *version;

public:
    BaseSensor(
        const char *sensor_id,
        const char *sensor_name,
        const char *sensor_type,
        const char *metadata = "{}",
        const char *version = "v0.4_arduino",
        const char *sensor_sub_type = nullptr);
    ~BaseSensor();

    virtual void init(char *mqtt_header, EspMQTTClient *mqtt_client);
    virtual void loop();
    void export_data(char *exported_data);
    bool export_data(char *exported_data, size_t size);
    void add_export_data(JsonObject exported_data);
    void construct_topic(char *newTopic, size_t size, const char *endpoint);
};

#endif // _IOTCLOUD_BASE_SENSOR
