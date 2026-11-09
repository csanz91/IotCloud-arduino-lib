#ifndef _IOTCLOUD_TOGGLE_SENSOR
#define _IOTCLOUD_TOGGLE_SENSOR

#include "EspMQTTClient.h"
#include "sensor.h"

class Toggle : public BaseSensor
{

public:
    enum ToggleState
    {
        INACTIVE,
        UP,
        DOWN,
        TOGGLE
    };

protected:
    virtual void set_toggle_state(ToggleState);
    unsigned long action_time = 10000;
    unsigned long action_start_time = 0;
    ToggleState toggle_action = ToggleState::INACTIVE;

    Toggle(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata = "{}",
        const char *sensor_sub_type = nullptr);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_TOGGLE_SENSOR
