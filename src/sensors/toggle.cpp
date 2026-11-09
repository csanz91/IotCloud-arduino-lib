#include "toggle.h"

Toggle::Toggle(
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata,
    const char *sensor_sub_type) : BaseSensor(sensor_id,
                                              sensor_name,
                                              "toggle",
                                              metadata,
                                              "v0.4_arduino",
                                              sensor_sub_type)
{
}

void Toggle::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/setToggle");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               if (payload.equals("toggle"))
                               {
                                   set_toggle_state(ToggleState::TOGGLE);
                               }
                               else if (payload.equals("up"))
                               {
                                   set_toggle_state(ToggleState::UP);
                               }
                               else if (payload.equals("down"))
                               {
                                   set_toggle_state(ToggleState::DOWN);
                               } });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/actionTime");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { action_time = atoi(payload.c_str()); });
};

void Toggle::set_toggle_state(ToggleState toggle_state)
{
    toggle_action = toggle_state;
    action_start_time = millis();
}
