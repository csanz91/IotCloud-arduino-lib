#include "analog.h"

AnalogSensor::AnalogSensor(
    const char *sensor_id,
    const char *sensor_name,
    const unsigned int report_value_period,
    const unsigned short decimals,
    const bool enable_filter,
    const char *sensor_type,
    const char *metadata,
    const float filter_rate_change,
    const char *sensor_sub_type) : BaseSensor(sensor_id,
                                                 sensor_name,
                                                 sensor_type,
                                                 metadata,
                                                 "v0.4_arduino",
                                                 sensor_sub_type),
                                      _report_value_period(report_value_period),
                                      _decimals(decimals),
                                      _enable_filter(enable_filter),
                                      _filter_rate_change(filter_rate_change)
{
    _lastTask = 0;
    _next_running_avg = 0;
    _elements_in_buffer = 0;
    value = NAN;
    offset = 0;
}

void AnalogSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/offset");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               float new_offset = atof(payload.c_str());
                               if (!isnan(new_offset) && !isinf(new_offset))
                                   offset = new_offset; });
}

bool AnalogSensor::report_value()
{
    // Make sure the client has been initialized
    if (!_mqtt_client || !_mqtt_client->isConnected())
        return false;

    // Don't publish NaN or Inf values
    if (isnan(value) || isinf(value))
        return false;

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "value");
    // dtostrf() does not check the buffer size and prints every integer digit.
    // A finite float has at most 39 integer digits, so with the sign, the
    // decimal point, up to 6 decimals and the terminator, 48 bytes always fit.
    char array[48];
    dtostrf(value, 1, _decimals > 6 ? 6 : _decimals, array);
    _mqtt_client->publish(constructedTopic, array, true);
    return true;
}

void AnalogSensor::get_value()
{
}
float AnalogSensor::filter_value(float new_value)
{
    // Check forbiden values
    if (isnan(new_value) || isinf(new_value)){
        return NAN;
    }

    if (!_enable_filter)
        return new_value;

    if (_elements_in_buffer > 0)
    {
        float running_avg_value = 0;
        for (int i = 0; i < _elements_in_buffer; ++i)
        {
            running_avg_value += _running_avg_buffer[i];
        }
        running_avg_value /= _elements_in_buffer;

        // Maximum allowed change. fabsf() keeps the window valid for negative
        // averages, and the floor stops it from collapsing near zero.
        float avg_tolerance = fabsf(running_avg_value) * _filter_rate_change;
        if (avg_tolerance < _filter_min_tolerance)
            avg_tolerance = _filter_min_tolerance;

        if (fabsf(new_value - running_avg_value) > avg_tolerance)
        {
            // Keep outliers out of the average so one glitch can't block the
            // good readings that follow it.
            if (++_consecutive_rejections < _filter_max_rejections)
                return NAN;

            // The value stayed out of range: treat it as a real change and
            // restart the average from it.
            _elements_in_buffer = 0;
            _next_running_avg = 0;
        }
    }
    _consecutive_rejections = 0;

    _running_avg_buffer[_next_running_avg++] = new_value;
    if (_next_running_avg >= _running_avg_count)
    {
        _next_running_avg = 0;
    }
    if (_elements_in_buffer < _running_avg_count)
    {
        _elements_in_buffer++;
    }

    return new_value;
}

bool AnalogSensor::set_value(float new_value)
{

    float filtered_value = filter_value(new_value);
    if (!isnan(filtered_value))
    {
        value = filtered_value + offset;
        return report_value();
    }
    return false;
}

void AnalogSensor::loop()
{
    unsigned int elapsed = millis() - _lastTask;
    if (elapsed > _report_value_period)
    {
        get_value();
        _lastTask = millis();
    }
}
