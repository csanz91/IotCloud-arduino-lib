#ifndef _IOTCLOUD_TOOGLE_SENSOR
#define _IOTCLOUD_TOOGLE_SENSOR

#include "EspMQTTClient.h"
#include "sensor.h"

class Toogle : public BaseSensor
{

public:
    enum ToogleStates
    {
        INACTIVE,
        UP,
        DOWN,
        TOOGLE
    };

protected:
    virtual void set_toogle_state(ToogleStates);
    unsigned long action_time = 10000;
    unsigned long action_start_time = 0;
    ToogleStates toogle_action = ToogleStates::INACTIVE;

    Toogle(
        const char *sensor_id,
        const char *sensor_name,
        const char *metadata = "{}");

    void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_TOOGLE_SENSOR