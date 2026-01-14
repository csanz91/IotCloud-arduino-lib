#include "switch.h"

SwitchSensor::SwitchSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type,
    const char *metadata) : BaseSensor(sensor_id,
                                       sensor_name,
                                       sensor_type,
                                       metadata)
{
}

void SwitchSensor::report_state()
{
    // Make sure the client has been initialized
    if (!_mqtt_client)
        return;
    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "state");
    _mqtt_client->publish(constructedTopic, state ? "true" : "false", true);
}

void SwitchSensor::set_state(bool new_state)
{
    state = new_state;
    report_state();
}

void SwitchSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "setState");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
                               Serial.println("New state received");
#endif
                               set_state(payload.equalsIgnoreCase("true")); });

    report_state();
};