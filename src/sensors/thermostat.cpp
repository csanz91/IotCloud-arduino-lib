#include "thermostat.h"

Thermostat::Thermostat(
    const char *sensor_id,
    const char *sensor_name,
    const unsigned int report_value_period,
    const unsigned short decimals,
    const bool enable_filter,
    const char *sensor_type,
    const char *metadata,
    const float filter_rate_change,
    const char *sensor_sub_type) : AnalogSensor(sensor_id,
                                                   sensor_name,
                                                   report_value_period,
                                                   decimals,
                                                   enable_filter,
                                                   sensor_type,
                                                   metadata,
                                                   filter_rate_change,
                                                   sensor_sub_type)
{
}

void Thermostat::report_state()
{
    if (!_mqtt_client)
        return;
    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "state");
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
    if (!_mqtt_client)
        return;
    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/heating");
    _mqtt_client->publish(constructedTopic, heating ? "true" : "false", true);
}

void Thermostat::loop()
{
    AnalogSensor::loop();

#if IOTCLOUD_THERMOSTAT_OFFLINE_HEATING_TIMEOUT_MS > 0
    // Heating is switched on and off from the cloud. If the connection is lost
    // while heating, turn it off after a while instead of leaving it on until
    // the connection comes back.
    if (!heating || (_mqtt_client && _mqtt_client->isConnected()))
    {
        _offline_since_ms = millis();
        return;
    }
    if (millis() - _offline_since_ms >= IOTCLOUD_THERMOSTAT_OFFLINE_HEATING_TIMEOUT_MS)
    {
        IOTCLOUD_LOG_PRINTLN(F("Thermostat offline while heating; turning heating off"));
        set_heating(false);
    }
#endif
}

void Thermostat::set_heating(bool new_heating)
{
    heating = new_heating;
    report_heating();
}

void Thermostat::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    AnalogSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "setState");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
                               IOTCLOUD_LOG_PRINTLN("New state received");
#endif
                               set_state(payload.equalsIgnoreCase("true"));
                           });
    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/setHeating");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
                               IOTCLOUD_LOG_PRINTLN("New heating state received");
#endif
                               set_heating(payload.equalsIgnoreCase("true"));
                           });

    report_state();
    report_heating();
};
