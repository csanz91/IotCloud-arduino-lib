#ifndef _IOTCLOUD_SWITCH_SENSOR
#define _IOTCLOUD_SWITCH_SENSOR

#include "EspMQTTClient.h"
#include "sensor.h"

class SwitchSensor : public BaseSensor
{

protected:
    bool state = false;
    virtual void set_state(bool);
    void report_state();

public:
    
    SwitchSensor(
        const char *sensor_id,
        const char *sensor_name,
        const char *sensor_type = "switch",
        const char *metadata = "{}");

    virtual void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_SWITCH_SENSOR