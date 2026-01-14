#ifndef _IOTCLOUD_BASE_SENSOR
#define _IOTCLOUD_BASE_SENSOR

#include "EspMQTTClient.h"
#include "../iotcloud_constants.h"

class BaseSensor
{
private:
    char _sensor_id[64];
    const char *_sensor_type;
    const char *_metadata;

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
        const char *version = "v0.4_arduino");
    ~BaseSensor();

    virtual void init(char *mqtt_header, EspMQTTClient *mqtt_client);
    virtual void loop();
    void export_data(char *exported_data);
    void construct_topic(char *newTopic, size_t size, const char *endpoint);
};

#endif // _IOTCLOUD_BASE_SENSOR