#ifndef _IOTCLOUD_LED_SENSOR
#define _IOTCLOUD_LED_SENSOR

#include "EspMQTTClient.h"
#include "switch.h"

class LedSensor : public SwitchSensor
{
protected:
    float _brightness = 0.0; // 1.0 max brightness
    float _initialBrightness = 0.5;
    virtual void set_brightness(float);
    void report_brightness();
    void set_state(bool new_state);

public:
        LedSensor(
        const char *sensor_id,
        const char *sensor_name,
        const char *sensor_type = "led",
        const char *metadata = "{}");

    void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_LED_SENSOR