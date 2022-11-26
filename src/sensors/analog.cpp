#include "analog.h"

AnalogSensor::AnalogSensor(
    const char *sensor_id,
    const char *sensor_name,
    const unsigned int report_value_period,
    const unsigned short decimals,
    const bool enable_filter,
    const char *sensor_type,
    const char *metadata,
    const float filter_rate_change) : BaseSensor(sensor_id,
                                                 sensor_name,
                                                 sensor_type,
                                                 metadata),
                                      _report_value_period(report_value_period),
                                      _decimals(decimals),
                                      _enable_filter(enable_filter),
                                      _filter_rate_change(filter_rate_change)
{
    _lastTask = 0;
}

void AnalogSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "aux/offset");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { offset = payload.toFloat(); });
}

void AnalogSensor::report_value()
{
    // Make sure the client has been initialized
    if (!_mqtt_client)
        return;
    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "value");
    char array[12];
    dtostrf(value, 1, _decimals, array);
    _mqtt_client->publish(constructedTopic, array, true);
}

void AnalogSensor::get_value()
{
}
float AnalogSensor::filter_value(float new_value)
{
    // Check forbiden values
    if (isnan(new_value) || isinf(new_value))
        return NULL;

    if (!_enable_filter)
        return new_value;

    // Calculate the running average
    _running_avg_buffer[_next_running_avg++] = new_value;
    if (_next_running_avg >= _running_avg_count)
    {
        _next_running_avg = 0;
    }
    if (_elements_in_buffer < _running_avg_count)
    {
        _elements_in_buffer++;
    }
    float running_avg_value = 0;
    for (int i = 0; i < _elements_in_buffer; ++i)
    {
        running_avg_value += _running_avg_buffer[i];
    }
    running_avg_value /= _elements_in_buffer;
    // Calculate the maximum allowed rate of change
    float avg_tolerance = running_avg_value * _filter_rate_change;

    // Check if the value is within the tolerances
    if (new_value < running_avg_value - avg_tolerance || new_value > running_avg_value + avg_tolerance)
        return NULL;

    return new_value;
}

void AnalogSensor::set_value(float new_value)
{

    float filtered_value = filter_value(new_value);
    if (filtered_value != NULL)
    {
        value = filtered_value + offset;
        report_value();
    }
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