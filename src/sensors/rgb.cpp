#include "rgb.h"

RgbSensor::RgbSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_sub_type) : LedSensor(sensor_id,
                                             sensor_name,
                                             "ledRGB",
                                             "{}",
                                             sensor_sub_type)
{
}

void RgbSensor::report_color()
{
    if (!_mqtt_client)
        return;
    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/color");
    _mqtt_client->publish(constructedTopic, _color, true);
}

void RgbSensor::set_color(const char *color)
{
    strlcpy(_color, color, sizeof(_color));
    report_color();
}

void RgbSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
#ifdef DEBUG
    IOTCLOUD_LOG_PRINTLN("Init rgb");
#endif
    LedSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/setColor");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN(F("New color value received"));
#endif

        set_color(payload.c_str()); });
};
