#include "runtime_mode.h"

EspMQTTClient *mqtt_client;

device_data data;
Device *_device;

void runtime_mode_setup(const device_data *received_data, Device *d)
{
    data = *received_data;
    _device = d;

    // This can surely be optimized
    sprintf(d->mqtt_header, "v1/%s/%s/", data.locationId, data.deviceId);
    _device->get_topic(d->mqtt_status_topic, "status");

    mqtt_client = new EspMQTTClient(
        data.wifiSSID,
        data.wifiPassword,
        IotCloud_Constants::MQTT_SERVER,
        data.token,    // Username
        "_",           // Password
        data.deviceId, // Client ID
        1883);

    mqtt_client->setMaxPacketSize(1024);

#ifdef DEBUG
    mqtt_client->enableDebuggingMessages();
#endif
    mqtt_client->enableLastWillMessage(_device->mqtt_status_topic, "offline", true);
}

void onConnectionEstablished()
{
    Serial.println(F("Connected to the mqtt server"));
    configTzTime(TZ_Europe_Madrid, IotCloud_Constants::NTP_SERVER);

    _device->init(mqtt_client);
    OTA::init(_device->mqtt_header, _device->device_current_version, _device->device_model, mqtt_client);
}

void runtime_mode_loop()
{
    mqtt_client->loop();
    _device->loop();
    OTA::loop();
}
