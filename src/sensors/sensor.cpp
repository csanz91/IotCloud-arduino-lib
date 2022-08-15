#include <ArduinoJson.h>

#include "sensor.h"

BaseSensor::BaseSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type,
    const char *metadata,
    const char *version) : _sensor_id(),
                           _sensor_name(sensor_name),
                           _sensor_type(sensor_type),
                           _metadata(metadata),
                           version(version)
{
    strcpy(_sensor_id, IotCloud_Constants::DEVICE_INTERNAL_ID);
    strcat(_sensor_id, "_");
    strcat(_sensor_id, sensor_id);
}

void BaseSensor::construct_topic(char *newTopic, const char *endpoint)
{
    strcpy(newTopic, _mqtt_header);
    strcat(newTopic, _sensor_id);
    strcat(newTopic, "/");
    strcat(newTopic, endpoint);
}

void BaseSensor::init(char *mqtt_header, EspMQTTClient *mqtt_client)
{
    _mqtt_client = mqtt_client;
    _mqtt_header = mqtt_header;
}

void BaseSensor::export_data(char *exported_data)
{
    const int jsonDocumentMaxSize = 512;
    StaticJsonDocument<jsonDocumentMaxSize> doc;

    doc[F("sensorId")] = _sensor_id;
    doc[F("sensorName")] = _sensor_name;
    doc[F("sensorType")] = _sensor_type;
    doc[F("version")] = version;
    doc[F("sensorMetadata")] = serialized(_metadata);

    size_t len = serializeJson(doc, exported_data, jsonDocumentMaxSize);
    assert(len < jsonDocumentMaxSize);
    doc.clear();
}

void BaseSensor::loop()
{
}

BaseSensor::~BaseSensor()
{
}