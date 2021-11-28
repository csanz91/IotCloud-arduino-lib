#include "thermostat.h"

Thermostat::Thermostat(
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

void Thermostat::report_value()
{
    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "value");
    char array[12];
    dtostrf(value, 1, _decimals, array);
    _mqtt_client->publish(constructedTopic, array, true);
}

void Thermostat::report_state()
{
    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "state");
    _mqtt_client->publish(constructedTopic, state ? "true" : "false", true);
}

void Thermostat::set_state(bool new_state)
{
    state = new_state;
    if (!state && heating)
    {
        set_heating(false);
    }
    report_state();
}

void Thermostat::report_heating()
{
    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "aux/heating");
    _mqtt_client->publish(constructedTopic, heating ? "true" : "false", true);
}

void Thermostat::set_heating(bool new_heating)
{
    heating = new_heating;
    report_heating();
}

void Thermostat::get_value()
{
}
float Thermostat::filter_value(float new_value)
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
    float running_avg_value = 0;
    for (int i = 0; i < _next_running_avg; ++i)
    {
        running_avg_value += _running_avg_buffer[i];
    }
    running_avg_value /= _next_running_avg;

    // Calculate the maximum allowed rate of change
    float avg_tolerance = running_avg_value * _filter_rate_change;

    // Check if the value is within the tolerances
    if (new_value < running_avg_value - avg_tolerance || new_value > running_avg_value + avg_tolerance)
        return NULL;

    return new_value;
}

void Thermostat::set_value(float new_value)
{

    float filtered_value = filter_value(new_value);
    if (filtered_value != NULL)
    {
        value = filtered_value;
        report_value();
    }
}

void Thermostat::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "aux/thermostat");
    mqtt_client->publish(constructedTopic, version, true);

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, "setState");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               Serial.println("New state received");
                               set_state(payload.equalsIgnoreCase("true"));
                           });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, "aux/setHeating");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               Serial.println("New heating state received");
                               set_heating(payload.equalsIgnoreCase("true"));
                           });

    report_state();
    report_heating();
};

void Thermostat::loop()
{
    unsigned int elapsed = millis() - _lastTask;
    if (elapsed > _report_value_period)
    {
        get_value();
        _lastTask = millis();
    }
}