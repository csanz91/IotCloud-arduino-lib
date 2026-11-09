#include <ArduinoJson.h>

#include "../IotCloud_config.h"
#include "sensor.h"

BaseSensor::BaseSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type,
    const char *metadata,
    const char *version,
    const char *sensor_sub_type) : _sensor_id_suffix(sensor_id),
                           _sensor_id(),
                           _sensor_name(sensor_name),
                           _sensor_type(sensor_type),
                           _sensor_sub_type(sensor_sub_type),
                           _metadata(metadata),
                           version(version)
{
    refresh_sensor_id();
}

void BaseSensor::refresh_sensor_id()
{
    if (IotCloud_Constants::DEVICE_INTERNAL_ID[0] == '\0')
    {
        IotCloud_Constants::setup_constants();
    }
    snprintf(_sensor_id, sizeof(_sensor_id), "%s_%s", IotCloud_Constants::DEVICE_INTERNAL_ID, _sensor_id_suffix);
}

void BaseSensor::construct_topic(char *newTopic, size_t size, const char *endpoint)
{
    snprintf(newTopic, size, "%s%s/%s", _mqtt_header, _sensor_id, endpoint);
}

void BaseSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    refresh_sensor_id();
    _mqtt_client = mqtt_client;
    _mqtt_header = mqtt_header;
}

void BaseSensor::export_data(char *exported_data)
{
    export_data(exported_data, IOTCLOUD_SENSOR_EXPORT_JSON_SIZE);
}

bool BaseSensor::export_data(char *exported_data, size_t size)
{
    DynamicJsonDocument doc(size);

    JsonObject sensor_data = doc.to<JsonObject>();
    add_export_data(sensor_data);

    if (doc.overflowed())
    {
        doc.clear();
        return false;
    }

    size_t len = serializeJson(doc, exported_data, size);
    bool exported = len < size;
    assert(exported);
    doc.clear();
    return exported;
}

void BaseSensor::add_export_data(JsonObject exported_data)
{
    refresh_sensor_id();
    exported_data[F("sensorId")] = _sensor_id;
    exported_data[F("sensorName")] = _sensor_name;
    exported_data[F("sensorType")] = _sensor_type;
    if (_sensor_sub_type && _sensor_sub_type[0] != '\0')
    {
        exported_data[F("sensorSubType")] = _sensor_sub_type;
    }
    exported_data[F("version")] = version;
    exported_data[F("sensorMetadata")] = serialized(_metadata);
}

void BaseSensor::loop()
{
}

BaseSensor::~BaseSensor()
{
}
