#ifndef _IOTCLOUD_RGB_SENSOR
#define _IOTCLOUD_RGB_SENSOR

#include "EspMQTTClient.h"
#include "led.h"

class RgbSensor : public LedSensor
{
private:
    char _color[9];
    void set_color(const char *color);
    void report_color();

public:
    RgbSensor(
        const char *sensor_id,
        const char *sensor_name);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_RGB_SENSOR