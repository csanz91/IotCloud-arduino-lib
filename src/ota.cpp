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

        ESPhttpUpdate.rebootOnUpdate(false);

        char constructedTopic[64] = "v1/ota/update/";
        strcat(constructedTopic, _device_model);
        mqtt_client->subscribe(constructedTopic, [&](const String &payload) {
            Serial.println("New ota update request received");
            if (strlcpy(_new_version, payload.c_str(), sizeof _new_version) >= sizeof _new_version)
            {
                _publishOtaStatus(400, "Version lenght too big");
                return;
            }

            Serial.println(_new_version);
            _ota_requested = true;
        });
    }

    bool _publishOtaStatus(int status, const char *info)
    {
        char payload[90] = "";
        itoa(status, payload, 10);
        if (info)
        {
            strcat(payload, " ");
            strcat(payload, info);
        }
        char topic[strlen(_mqtt_ota_header) + 11];
        strcpy(topic, _mqtt_ota_header);
        strcat(topic, "ota/status");
        _mqtt_client->publish(topic, payload);
    }

    void _start_update()
    {
        Serial.println(F("Starting OTA update"));

        if (strcmp(_new_version, _current_version) == 0)
        {
            Serial.println(F("The device has already been updated. Ignore update"));
            _publishOtaStatus(400, "The device has already been updated. Ignore update");
            return;
        }

        X509List cert(LETSENCRYPT_ROOT_CA);

        WiFiClientSecure client;
        client.setTrustAnchors(&cert);
        if (!client.connect(IotCloud_Constants::OTA_SERVER, 443))
        {
            _publishOtaStatus(400, "Connection failed");
            Serial.println(F("Connection failed"));
            return;
        }

        char url[128] = "https://";
        strcat(url, IotCloud_Constants::OTA_SERVER);
        strcat(url, "/");
        strcat(url, _device_model);
        strcat(url, "/");
        strcat(url, _new_version);

        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

        t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);

        char error_msg[80];
        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            sprintf(error_msg, "HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            Serial.println(error_msg);
            _publishOtaStatus(400, error_msg);
            break;

        case HTTP_UPDATE_NO_UPDATES:
            strcpy(error_msg, "HTTP_UPDATE_NO_UPDATES");
            Serial.println(error_msg);
            _publishOtaStatus(400, error_msg);
            break;

        case HTTP_UPDATE_OK:
            strcpy(error_msg, "Update finished suscesfully. Restarting...");
            Serial.println(error_msg);
            _publishOtaStatus(400, error_msg);
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