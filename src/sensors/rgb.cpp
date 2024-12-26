#include "rgb.h"

RgbSensor::RgbSensor(
    const char *sensor_id,
    const char *sensor_name) : LedSensor(sensor_id,
                                         sensor_name,
                                         "ledRGB")
{
}

void RgbSensor::report_color()
{
    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "aux/color");
    _mqtt_client->publish(constructedTopic, _color, true);
}

void RgbSensor::set_color(const char *color)
{
    strcpy(_color, color);
    report_color();
}

void RgbSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
#ifdef DEBUG
    Serial.println("Init rgb");
#endif
    LedSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[104] = "";
    construct_topic(constructedTopic, "aux/setColor");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
        Serial.println(F("New color value received"));
#endif

        set_color(payload.c_str()); });
};