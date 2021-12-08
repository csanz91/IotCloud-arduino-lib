
#include "toogle.hpp"

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
    Serial.println("Init toogle");
    BaseSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[104] = "";
    construct_topic(constructedTopic, "aux/setToogle");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload) {
        Serial.println("New toogle received");
        Serial.println(payload);
    });
};
