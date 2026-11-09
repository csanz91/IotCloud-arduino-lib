#include "led.h"

// Keep brightness in the documented 0.0-1.0 range. Returns NAN for
// unparseable input so callers can ignore it.
static float clamp_brightness(float brightness)
{
    if (isnan(brightness))
        return NAN;
    if (brightness < 0.0f)
        return 0.0f;
    if (brightness > 1.0f)
        return 1.0f;
    return brightness;
}

LedSensor::LedSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type,
    const char *metadata,
    const char *sensor_sub_type) : SwitchSensor(sensor_id,
                                                sensor_name,
                                                sensor_type,
                                                metadata,
                                                sensor_sub_type)
{
}

void LedSensor::report_brightness()
{
    if (!_mqtt_client)
        return;
    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/brightness");
    // _brightness is clamped to 0.0-1.0, so this is at most " 1.00"
    char array[8];
    dtostrf(_brightness, 5, 2, array);
    _mqtt_client->publish(constructedTopic, array, true);
}

void LedSensor::set_brightness(float brightness)
{
    brightness = clamp_brightness(brightness);
    if (isnan(brightness))
        return;
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

    char constructedTopic[128] = "";
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/setBrightness");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               // Clamp here too so subclasses overriding set_brightness() get a safe value
                               float brightness = clamp_brightness(atof(payload.c_str()));
                               if (!isnan(brightness))
                                   set_brightness(brightness); });

    constructedTopic[0] = '\0';
    construct_topic(constructedTopic, sizeof(constructedTopic), "aux/initialBrightness");
    mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                           {
                               float brightness = clamp_brightness(atof(payload.c_str()));
                               if (!isnan(brightness))
                                   _initialBrightness = brightness; });

    report_brightness();
};
