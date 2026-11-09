#ifndef _IOTCLOUD_THERMOSTAT_SENSOR
#define _IOTCLOUD_THERMOSTAT_SENSOR

#include "analog.h"

class Thermostat : public AnalogSensor
{

protected:
    bool state = false;
    virtual void set_state(bool);
    void report_state();

    bool heating = false;
    virtual void set_heating(bool);
    void report_heating();

private:
    // When the connection was lost while heating (see loop())
    unsigned long _offline_since_ms = 0;

public:
    Thermostat(
        const char *sensor_id,
        const char *sensor_name,
        const unsigned int report_value_period = 30000, // milliseconds
        const unsigned short decimals = 2,
        const bool enable_filter = true,
        const char *sensor_type = "thermostat",
        const char *metadata = "{}",
        const float filter_rate_change = 0.1,
        const char *sensor_sub_type = nullptr);

    virtual void init(char *mqtt_header, EspMQTTClient *mqtt_client);
    void loop();
};

#endif // _IOTCLOUD_THERMOSTAT_SENSOR
