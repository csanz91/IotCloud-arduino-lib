#include "led.h"

LedSensor::LedSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type,
    const char *metadata) : SwitchSensor(sensor_id,
                                         sensor_name,
                                         sensor_type,
                                         metadata)
{
}

void LedSensor::report_brightness()
{
    char constructedTopic[94] = "";
    construct_topic(constructedTopic, "aux/brightness");
    char array[7];
    dtostrf(_brightness, 5, 2, array);
    _mqtt_client->publish(constructedTopic, array, true);
}

void LedSensor::set_brightness(float brightness)
{
    _brightness = brightness;

    bool new_state = brightness > 0.0;
    if (new_state != state)
        SwitchSensor::set_state(new_state);

    report_brightness();
}

void LedSensor::set_state(bool new_state)
{
    if (new_state && _brightness == 0.0)
    {
        set_brightness(_initialBrightness);
    }
    else if (!new_state && _brightness > 0.0)
    {
        set_brightness(0.0);
    }
}

void LedSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    SwitchSensor::init(mqtt_header, mqtt_client);

    char constructedTopic[110] = "";
    construct_topic(constructedTopic, "aux/setBrightness");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { set_brightness(atof(payload.c_str())); });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, "aux/initialBrightness");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           { _initialBrightness = atof(payload.c_str()); });

    report_brightness();
};