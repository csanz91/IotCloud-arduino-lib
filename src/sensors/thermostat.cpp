#include "thermostat.h"

Thermostat::Thermostat(
    const char *sensor_id,
    const char *sensor_name,
    const unsigned int report_value_period,
    const unsigned short decimals,
    const bool enable_filter,
    const char *sensor_type,
    const char *metadata,
    const float filter_rate_change) : AnalogSensor(sensor_id,
                                                   sensor_name,
                                                   report_value_period,
                                                   decimals,
                                                   enable_filter,
                                                   sensor_type,
                                                   metadata,
                                                   filter_rate_change)
{
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
    char constructedTopic[104] = "";
    construct_topic(constructedTopic, "aux/heating");
    _mqtt_client->publish(constructedTopic, heating ? "true" : "false", true);
}

void Thermostat::set_heating(bool new_heating)
{
    heating = new_heating;
    report_heating();
}

void Thermostat::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    AnalogSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[104] = "";
    construct_topic(constructedTopic, "setState");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
                               Serial.println("New state received");
#endif
                               set_state(payload.equalsIgnoreCase("true"));
                           });
    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, "aux/setHeating");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
                               Serial.println("New heating state received");
#endif
                               set_heating(payload.equalsIgnoreCase("true"));
                           });

    report_state();
    report_heating();
};