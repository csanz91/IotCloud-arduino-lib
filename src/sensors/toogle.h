#ifndef _IOTCLOUD_TOOGLE_SENSOR
#define _IOTCLOUD_TOOGLE_SENSOR

#include "EspMQTTClient.h"
#include "sensor.h"

class Toogle : public BaseSensor
{

public:
    enum ToogleStates
    {
        UP,
        DOWN,
        TOOGLE
    };

    Toogle(
        const char *sensor_id,
        const char *sensor_name);

    void init(char *mqtt_header, EspMQTTClient *mqtt_client);

protected:
    virtual void set_toogle_state(ToogleStates);
};

#endif // _IOTCLOUD_TOOGLE_SENSOR