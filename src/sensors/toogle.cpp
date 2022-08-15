#include "toogle.h"

Toogle::Toogle(
    const char *sensor_id,
    const char *sensor_name) : BaseSensor(sensor_id,
                                          sensor_name,
                                          "toogle",
                                          "{\"bidirectional\": false}")
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
};

void Toogle::set_toogle_state(ToogleStates toogle_state)
{
    Serial.print("New toogle received: ");
    Serial.println(toogle_state);
}
