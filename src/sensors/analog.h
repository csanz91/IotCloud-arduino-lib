#ifndef _IOTCLOUD_ANALOG_SENSOR
#define _IOTCLOUD_ANALOG_SENSOR

#include "sensor.h"

class AnalogSensor : public BaseSensor
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
    float value;
    const unsigned short _decimals;
    virtual void get_value();
    float filter_value(float);
    virtual void set_value(float);
    virtual void report_value();

public:
    AnalogSensor(
        const char *sensor_id,
        const char *sensor_name,
        const unsigned int report_value_period = 30000, // milliseconds
        const unsigned short decimals = 2,
        const bool enable_filter = true,
        const char *sensor_type = "analog",
        const char *metadata = "{}",
        const float filter_rate_change = 0.1);

    void loop();
};

#endif // _IOTCLOUD_ANALOG_SENSOR