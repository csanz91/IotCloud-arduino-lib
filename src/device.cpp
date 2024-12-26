#include <ArduinoJson.h>

#include "device.h"

Device::Device(
    const char *device_current_version,
    const char *device_model) : device_current_version(device_current_version), device_model(device_model)
{
    if (IotCloud_Constants::DEVICE_INTERNAL_ID[0] == '\0')
        IotCloud_Constants::setup_constants();

    _deviceInternalId = IotCloud_Constants::DEVICE_INTERNAL_ID;
    char mqtt_header[70] = "";
    char mqtt_status_topic[76] = "";
}

void Device::init(EspMQTTClient *mqtt_client)
{

    mqtt_client->publish(mqtt_status_topic, "online", true);

    char topic[strlen(mqtt_header) + 8];
    get_topic(topic, "ip");
    mqtt_client->publish(topic, WiFi.localIP().toString(), true);

    get_topic(topic, "version");
    mqtt_client->publish(topic, device_current_version, true);

    get_topic(topic, "model");
    mqtt_client->publish(topic, device_model, true);

    get_topic(topic, "reset");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        if (payload.equalsIgnoreCase("true"))
        {
#ifdef DEBUG
            Serial.println(F("Reseting the device..."));
#endif
            clear_saved_data();
            ESP.restart();
        }
    });

    get_topic(topic, "restart");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        if (payload.equalsIgnoreCase("true"))
        {
            ESP.restart();
        }
    });

    for (const auto &sensor : _sensors)
    {
        sensor->init(mqtt_header, mqtt_client);
    }
}

void Device::export_data(char *exported_data)
{

    const int jsonDocumentMaxSize = 1024;

    StaticJsonDocument<jsonDocumentMaxSize> doc;

    doc[F("deviceInternalId")] = _deviceInternalId;
    doc[F("deviceVersion")] = device_current_version;
    JsonArray json_sensors = doc.createNestedArray("sensors");

    for (const auto &sensor : _sensors)
    {
        char sensor_exported_data[512];
        sensor->export_data(sensor_exported_data);
        json_sensors.add(serialized(sensor_exported_data));
    }
    size_t len = serializeJson(doc, exported_data, jsonDocumentMaxSize);
    assert(len < jsonDocumentMaxSize);
    doc.clear();
}

void Device::get_topic(char *topic, const char *endpoint)
{
    strcpy(topic, mqtt_header);
    strcat(topic, endpoint);
}

void Device::loop()
{
    for (const auto &sensor : _sensors)
    {
        sensor->loop();
    }
}

Device::~Device()
{
}