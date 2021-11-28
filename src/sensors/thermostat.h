#ifndef _IOTCLOUD_THERMOSTAT_SENSOR
#define _IOTCLOUD_THERMOSTAT_SENSOR

#include "sensor.h"

class Thermostat : public BaseSensor
{

private:
    unsigned int _lastTask;
    static const int _running_avg_count = 10;
    float _running_avg_buffer[_running_avg_count];
    int _next_running_avg;
    bool _enable_filter;
    const unsigned int _report_value_period;
    const float _filter_rate_change;

protected:
    bool state = false;
    virtual void set_state(bool);
    void report_state();

    bool heating = false;
    virtual void set_heating(bool);
    void report_heating();

    float value;
    const unsigned short _decimals;
    virtual void get_value();
    float filter_value(float);
    virtual void set_value(float);
    virtual void report_value();

public:
    Thermostat(
        const char *sensor_id,
        const char *sensor_name,
        const unsigned int report_value_period = 30000, // milliseconds
        const unsigned short decimals = 2,
        const bool enable_filter = true,
        const char *sensor_type = "thermostat",
        const char *metadata = "{}",
        const float filter_rate_change = 0.1);

    void loop();
    virtual void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_THERMOSTAT_SENSOR