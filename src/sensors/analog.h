#ifndef _IOTCLOUD_ANALOG_SENSOR
#define _IOTCLOUD_ANALOG_SENSOR

#include "sensor.h"

class AnalogSensor : public BaseSensor
{

private:
    unsigned int _lastTask;
    static const int _running_avg_count = 10;
    float _running_avg_buffer[_running_avg_count];
    int _next_running_avg = 0;
    int _elements_in_buffer = 0;
    // Readings in a row outside the tolerance before the filter accepts them
    // as a real change instead of a glitch.
    static const int _filter_max_rejections = 3;
    int _consecutive_rejections = 0;
    bool _enable_filter;
    const unsigned int _report_value_period;
    const float _filter_rate_change;

protected:
    float value;
    float offset;
    const unsigned short _decimals;
    // Smallest allowed change between readings, in the sensor's units, so the
    // filter still accepts small changes when the average is close to zero.
    float _filter_min_tolerance = 0.5;
    virtual void get_value();
    float filter_value(float);
    virtual bool set_value(float);
    virtual bool report_value();

public:
    AnalogSensor(
        const char *sensor_id,
        const char *sensor_name,
        const unsigned int report_value_period = 30000, // milliseconds
        const unsigned short decimals = 2,
        const bool enable_filter = true,
        const char *sensor_type = "analog",
        const char *metadata = "{}",
        const float filter_rate_change = 0.1,
        const char *sensor_sub_type = nullptr);

    void loop();
    void init(char *mqtt_header, EspMQTTClient *mqtt_client);
};

#endif // _IOTCLOUD_ANALOG_SENSOR
