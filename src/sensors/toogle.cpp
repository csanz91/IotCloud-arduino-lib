#include "toogle.h"

Toogle::Toogle(
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata) : BaseSensor(sensor_id,
                                       sensor_name,
                                       "toogle",
                                       metadata)
{
}

void Toogle::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[104] = "";
    construct_topic(constructedTopic, "aux/setToogle");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               if (payload.equals("toogle"))
                               {
                                   set_toogle_state(ToogleStates::TOOGLE);
                               }
                               else if (payload.equals("up"))
                               {
                                   set_toogle_state(ToogleStates::UP);
                               }
                               else if (payload.equals("down"))
                               {
                                   set_toogle_state(ToogleStates::DOWN);
                               } });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, "aux/actionTime");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { action_time = atoi(payload.c_str()); });
};

void Toogle::set_toogle_state(ToogleStates toogle_state)
{
    toogle_action = toogle_state;
    action_start_time = millis();
}
