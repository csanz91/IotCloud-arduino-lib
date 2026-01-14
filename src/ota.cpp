#include "ota.h"

namespace OTA
{
    namespace
    {
        const char *_mqtt_ota_header;
        const char *_current_version;
        const char *_device_model;
        char _new_version[64];
        bool _ota_requested;
        EspMQTTClient *_mqtt_client;
    } // namespace

    void init(const char *mqtt_ota_header, const char *current_version, const char *device_model, EspMQTTClient *mqtt_client)
    {
        if (!IotCloud_Constants::ENABLE_OTA)
            return;

        _mqtt_ota_header = mqtt_ota_header;
        _mqtt_client = mqtt_client;
        _current_version = current_version;
        _device_model = device_model;

#ifdef ESP8266
        ESPhttpUpdate.rebootOnUpdate(false);
#elif defined(ARDUINO_ARCH_ESP32)
        httpUpdate.rebootOnUpdate(false);
#endif

        char constructedTopic[128];
        snprintf(constructedTopic, sizeof(constructedTopic), "v1/ota/update/%s", _device_model);
        mqtt_client->subscribe(constructedTopic, [&](const String &payload)
                               {
#ifdef DEBUG
                                   Serial.println("New ota update request received");
#endif
                                   if (strlcpy(_new_version, payload.c_str(), sizeof _new_version) >= sizeof _new_version)
                                   {
                                       _publishOtaStatus(400, "Version length too big");
                                       return;
                                   }
#ifdef DEBUG
                                   Serial.println(_new_version);
#endif
                                   _ota_requested = true; });
    }

    void _publishOtaStatus(int status, const char *info)
    {
        char payload[128];
        if (info) {
            snprintf(payload, sizeof(payload), "%d %s", status, info);
        } else {
            snprintf(payload, sizeof(payload), "%d", status);
        }

        char topic[128];
        snprintf(topic, sizeof(topic), "%sota/status", _mqtt_ota_header);
        _mqtt_client->publish(topic, payload);
    }

    void _start_update()
    {
#ifdef DEBUG
        Serial.println(F("Starting OTA update"));
#endif

        if (strcmp(_new_version, _current_version) == 0)
        {
#ifdef DEBUG
            Serial.println(F("The device has already been updated. Ignore update"));
#endif
            _publishOtaStatus(400, "The device has already been updated. Ignore update");
            return;
        }

#ifdef ESP8266
        WiFiClientSecure client;
        X509List cert(IOTCLOUD_OTA_ROOT_CA);
        client.setTrustAnchors(&cert);
#elif defined(ARDUINO_ARCH_ESP32)
        NetworkClientSecure client;
        client.setCACert(IOTCLOUD_OTA_ROOT_CA);
#endif
        client.setTimeout(5000);
        if (!client.connect(IotCloud_Constants::OTA_SERVER, 443)) {
            char buf[200];
            #ifdef ESP8266
            int lastErr = client.getLastSSLError(buf, sizeof(buf));
            #elif defined(ARDUINO_ARCH_ESP32)
            int lastErr = client.lastError(buf, sizeof(buf));
            #endif
            
            _publishOtaStatus(400, buf);
            return;
        }

        char url[256];
        snprintf(url, sizeof(url), "https://%s/%s/%s", IotCloud_Constants::OTA_SERVER, _device_model, _new_version);

#ifdef ESP8266
        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
        t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
#elif defined(ARDUINO_ARCH_ESP32)
        httpUpdate.setLedPin(LED_BUILTIN, LOW);
        t_httpUpdate_return ret = httpUpdate.update(client, url);
#endif

        char error_msg[80];
        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
#ifdef ESP8266
            snprintf(error_msg, sizeof(error_msg), "HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());

#elif defined(ARDUINO_ARCH_ESP32)
            snprintf(error_msg, sizeof(error_msg), "HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
#endif
#ifdef DEBUG
            Serial.println(error_msg);
#endif
            _publishOtaStatus(400, error_msg);
            break;

        case HTTP_UPDATE_NO_UPDATES:
            strlcpy(error_msg, "HTTP_UPDATE_NO_UPDATES", sizeof(error_msg));
#ifdef DEBUG
            Serial.println(error_msg);
#endif
            _publishOtaStatus(400, error_msg);
            break;

        case HTTP_UPDATE_OK:
            strlcpy(error_msg, "Update finished successfully. Restarting...", sizeof(error_msg));
#ifdef DEBUG
            Serial.println(error_msg);
#endif
            _publishOtaStatus(200, error_msg);
            ESP.restart();
            break;
        }
    }

    void loop()
    {
        if (IotCloud_Constants::ENABLE_OTA && _ota_requested)
        {
            _start_update();
            _ota_requested = false;
        }
    }
} // namespace OTA