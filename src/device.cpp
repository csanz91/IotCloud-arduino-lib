#include <ArduinoJson.h>

#include "IotCloud_config.h"
#include "device.h"
#include "runtime_mode.h"

Device::Device(
    const char *device_current_version,
    const char *device_model) : device_current_version(device_current_version), device_model(device_model)
{
    _deviceInternalId = IotCloud_Constants::DEVICE_INTERNAL_ID;
    mqtt_header[0] = '\0';
    mqtt_status_topic[0] = '\0';
}

void Device::init(EspMQTTClient *mqtt_client)
{

    mqtt_client->publish(mqtt_status_topic, "online", true);

    char topic[128];
    get_topic(topic, sizeof(topic), "ip");
    mqtt_client->publish(topic, WiFi.localIP().toString(), true);

    get_topic(topic, sizeof(topic), "version");
    mqtt_client->publish(topic, device_current_version, true);

    get_topic(topic, sizeof(topic), "model");
    mqtt_client->publish(topic, device_model, true);

    get_topic(topic, sizeof(topic), "lib_version");
    mqtt_client->publish(topic, IotCloud_Constants::LIB_VERSION, true);

    get_topic(topic, sizeof(topic), "reboot_reason");
#ifdef ESP8266
    mqtt_client->publish(topic, ESP.getResetReason(), true);
#elif defined(ARDUINO_ARCH_ESP32)
    {
        const char *reset_reason;
        switch (esp_reset_reason())
        {
        case ESP_RST_POWERON:   reset_reason = "Power On";           break;
        case ESP_RST_EXT:       reset_reason = "External Pin";       break;
        case ESP_RST_SW:        reset_reason = "Software Restart";   break;
        case ESP_RST_PANIC:     reset_reason = "Exception/Panic";    break;
        case ESP_RST_INT_WDT:   reset_reason = "Interrupt Watchdog"; break;
        case ESP_RST_TASK_WDT:  reset_reason = "Task Watchdog";      break;
        case ESP_RST_WDT:       reset_reason = "Watchdog";           break;
        case ESP_RST_DEEPSLEEP: reset_reason = "Deep Sleep Wake";    break;
        case ESP_RST_BROWNOUT:  reset_reason = "Brownout";           break;
        default:                reset_reason = "Unknown";            break;
        }
        mqtt_client->publish(topic, reset_reason, true);
    }
#endif

    get_topic(topic, sizeof(topic), "reset");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        if (payload.equalsIgnoreCase("true"))
        {
#ifdef DEBUG
            IOTCLOUD_LOG_PRINTLN(F("Reseting the device..."));
#endif
            clear_saved_data();
            ESP.restart();
        }
    });

    get_topic(topic, sizeof(topic), "restart");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        if (payload.equalsIgnoreCase("true"))
        {
            ESP.restart();
        }
    });

    get_topic(topic, sizeof(topic), "network");
    mqtt_client->subscribe(topic, [&](const String &payload) {
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err)
        {
#ifdef DEBUG
            IOTCLOUD_LOG_PRINT(F("Network config JSON parse failed: "));
            IOTCLOUD_LOG_PRINTLN(err.c_str());
#endif
            return;
        }

        if (!doc["wifiSSID"].is<const char *>() || !doc["wifiPassword"].is<const char *>())
        {
#ifdef DEBUG
            IOTCLOUD_LOG_PRINTLN(F("Network config payload missing wifiSSID or wifiPassword"));
#endif
            return;
        }

        // Validates, keeps the current credentials as a fallback, saves and restarts
        runtime_mode_network_update(doc["wifiSSID"], doc["wifiPassword"]);
    });

    for (const auto &sensor : _sensors)
    {
        sensor->init(mqtt_header, mqtt_client);
    }
}

void Device::export_data(char *exported_data)
{
    export_data(exported_data, IOTCLOUD_DEVICE_EXPORT_JSON_SIZE);
}

bool Device::export_data(char *exported_data, size_t size)
{
    DynamicJsonDocument doc(size);

    doc[F("deviceInternalId")] = _deviceInternalId;
    doc[F("deviceVersion")] = device_current_version;
    doc[F("libVersion")] = IotCloud_Constants::LIB_VERSION;
    JsonArray json_sensors = doc.createNestedArray("sensors");

    for (const auto &sensor : _sensors)
    {
        JsonObject sensor_data = json_sensors.createNestedObject();
        sensor->add_export_data(sensor_data);
    }
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

void Device::get_topic(char *topic, size_t size, const char *endpoint)
{
    snprintf(topic, size, "%s%s", mqtt_header, endpoint);
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
