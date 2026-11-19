#include <ArduinoJson.h>

#include "ota.h"

#ifdef ARDUINO_ARCH_ESP32
#include <esp_ota_ops.h>
#include <nvs.h>

// The core marks a new OTA image valid as soon as it boots, so an image that
// crashes or can't get online later would stay. Keep it pending instead;
// OTA::loop() confirms it once healthy (see IOTCLOUD_OTA_CONFIRM_AFTER_MS).
extern "C" bool verifyRollbackLater() { return true; }
#endif

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
        bool _mqtt_connected_again; // Set by init(), which runs on every MQTT connect.

        // MQTT keepalive is 15 s (PubSubClient default) and the broker drops a
        // client after 1.5x that. A download longer than this has most likely
        // lost the connection, so reconnect before reporting the result.
        const unsigned long MQTT_SAFE_SILENCE_MS = 10000;
        const unsigned long MQTT_RECONNECT_TIMEOUT_MS = 15000;

        void _reconnectAfterLongDownload(unsigned long download_ms)
        {
            if (download_ms < MQTT_SAFE_SILENCE_MS)
                return;
            // Retry right away instead of the client's default 15 s. Not
            // restored: every path after a download ends in a status and, for
            // a new image, a restart.
            _mqtt_client->setMqttReconnectionAttemptDelay(500);
            _mqtt_connected_again = false;
            const unsigned long start = millis();
            while (!_mqtt_connected_again && millis() - start < MQTT_RECONNECT_TIMEOUT_MS)
            {
                _mqtt_client->loop();
                delay(10);
            }
        }
#ifdef ARDUINO_ARCH_ESP32
        enum class ImageState : uint8_t
        {
            Unknown,
            Pending,
            Settled
        };
        ImageState _image_state = ImageState::Unknown;
        bool (*_health_check)() = nullptr;
        unsigned long _mqtt_up_since_ms = 0; // 0 while disconnected

        // NVS key holding the version being installed, written just before the
        // reboot into it and removed once it is confirmed. If the device is
        // back on an older image with that key still set, that version rolled
        // back. (The image's own app descriptor can't tell: with the Arduino
        // core it holds the core build's version, not the firmware's.)
        const char *OTA_PREFS_NAMESPACE = "iotcloud_ota";
        const char *OTA_PREFS_INSTALLING = "installing";

        // Raw NVS API: Preferences adds ~3 KB, too much for the tightest builds.
        void _setInstallingVersion(const char *version)
        {
            nvs_handle_t handle;
            if (nvs_open(OTA_PREFS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
                return;
            if (version)
                nvs_set_str(handle, OTA_PREFS_INSTALLING, version);
            else
                nvs_erase_key(handle, OTA_PREFS_INSTALLING);
            nvs_commit(handle);
            nvs_close(handle);
        }

        // True if `version` was installed here before and rolled back.
        bool _rolledBackFrom(const char *version)
        {
            if (esp_ota_get_last_invalid_partition() == nullptr)
                return false;
            nvs_handle_t handle;
            if (nvs_open(OTA_PREFS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
                return false;
            char installing[64];
            size_t length = sizeof installing;
            const bool found = nvs_get_str(handle, OTA_PREFS_INSTALLING, installing, &length) == ESP_OK;
            nvs_close(handle);
            return found && strcmp(installing, version) == 0 &&
                   strcmp(installing, _current_version) != 0;
        }
#endif

#ifdef ESP8266
        // Crash guard. The ESP8266 has no rollback, and the update request is
        // retained, so a version whose download crashes the device (e.g. OOM in
        // the TLS handshake) is retried on every boot: a reboot loop. Mark the
        // attempt in RTC user memory, which survives exceptions, watchdog
        // resets and ESP.restart() but not a power cut (unplugging is a manual
        // retry). A marker still there when the next request comes in means
        // the last attempt never returned. After OTA_MAX_CRASHES of them the
        // version is refused with 410, which makes the server drop the request.
        // A different (newer) version starts a fresh count.
        //
        // User blocks 32-39 (bytes 128-159): eboot's command starts at user
        // byte 256 (0x60001200).
        const uint32_t OTA_ATTEMPT_RTC_BLOCK = 32;
        const uint32_t OTA_ATTEMPT_MAGIC = 0x4F544131; // "OTA1"
        const uint8_t OTA_MAX_CRASHES = 2;

        struct OtaAttempt
        {
            uint32_t magic;
            uint8_t crashes;
            char version[27];
        };
        static_assert(sizeof(OtaAttempt) % 4 == 0, "RTC memory is accessed in 4-byte blocks");

        // Returns the crashes recorded for `version` (0 if none).
        uint8_t _crashesFor(const char *version)
        {
            OtaAttempt attempt;
            if (!ESP.rtcUserMemoryRead(OTA_ATTEMPT_RTC_BLOCK, reinterpret_cast<uint32_t *>(&attempt), sizeof attempt))
                return 0;
            attempt.version[sizeof attempt.version - 1] = '\0';
            if (attempt.magic != OTA_ATTEMPT_MAGIC || strcmp(attempt.version, version) != 0)
                return 0;
            return attempt.crashes;
        }

        void _markAttempt(const char *version, uint8_t crashes)
        {
            OtaAttempt attempt = {};
            attempt.magic = OTA_ATTEMPT_MAGIC;
            attempt.crashes = crashes;
            strlcpy(attempt.version, version, sizeof attempt.version);
            ESP.rtcUserMemoryWrite(OTA_ATTEMPT_RTC_BLOCK, reinterpret_cast<uint32_t *>(&attempt), sizeof attempt);
        }

        void _clearAttempt()
        {
            OtaAttempt attempt = {};
            ESP.rtcUserMemoryWrite(OTA_ATTEMPT_RTC_BLOCK, reinterpret_cast<uint32_t *>(&attempt), sizeof attempt);
        }
#endif
    } // namespace

    void init(const char *mqtt_ota_header, const char *current_version, const char *device_model, EspMQTTClient *mqtt_client)
    {
        // Stored even with OTA off: the image confirmation needs it.
        _mqtt_client = mqtt_client;
        _mqtt_connected_again = true;
        if (!IotCloud_Constants::ENABLE_OTA)
            return;

        _mqtt_ota_header = mqtt_ota_header;
        _current_version = current_version;
        _device_model = device_model;

#ifdef ESP8266
        ESPhttpUpdate.rebootOnUpdate(false);
#elif defined(ARDUINO_ARCH_ESP32)
        httpUpdate.rebootOnUpdate(false);
#endif

        // Model-wide releases reach every device of the model. The per-device
        // topic carries staged rollouts; the server publishes it retained so a
        // device that was offline still gets it, and clears it after success.
        char constructedTopic[128];
        snprintf(constructedTopic, sizeof(constructedTopic), "v1/ota/update/%s", _device_model);
        mqtt_client->subscribe(constructedTopic, _onUpdateRequest);
        snprintf(constructedTopic, sizeof(constructedTopic), "%sota/update", _mqtt_ota_header);
        mqtt_client->subscribe(constructedTopic, _onUpdateRequest);
    }

    void _onUpdateRequest(const String &payload)
    {
        // An empty payload is the server clearing the retained request.
        if (payload.length() == 0)
            return;
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN("New ota update request received");
#endif
        if (strlcpy(_new_version, payload.c_str(), sizeof _new_version) >= sizeof _new_version)
        {
            _publishOtaStatus(400, "Version length too big");
            return;
        }
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN(_new_version);
#endif
        _ota_requested = true;
    }

    // Reads "vX.Y.Z" or "X.Y.Z"; anything after the patch number is ignored.
    bool _parseVersion(const char *version, unsigned long parts[3])
    {
        if (*version == 'v' || *version == 'V')
            version++;
        for (int i = 0; i < 3; i++)
        {
            if (!isdigit((unsigned char)*version))
                return false;
            char *end;
            parts[i] = strtoul(version, &end, 10);
            version = end;
            if (i < 2)
            {
                if (*version != '.')
                    return false;
                version++;
            }
        }
        return true;
    }

    // <0: older than the running firmware, 0: the same, >0: newer. A version
    // that can't be parsed counts as newer, so a typo never blocks updates.
    int _compareToCurrent(const char *requested)
    {
        if (strcmp(requested, _current_version) == 0)
            return 0;
        unsigned long req[3], cur[3];
        if (!_parseVersion(requested, req) || !_parseVersion(_current_version, cur))
            return 1;
        for (int i = 0; i < 3; i++)
        {
            if (req[i] != cur[i])
                return req[i] > cur[i] ? 1 : -1;
        }
        return 1; // Same X.Y.Z with a different suffix (pre-release builds).
    }

    void _publishOtaStatus(int status, const char *info)
    {
        StaticJsonDocument<384> doc;
        doc[F("code")] = status;
        if (info)
            doc[F("message")] = info;
        if (_new_version[0] != '\0')
            doc[F("requestedVersion")] = _new_version;

        char payload[384];
        size_t payload_size = serializeJson(doc, payload, sizeof(payload));
        if (payload_size == 0 || payload_size >= sizeof(payload))
            return;

        char topic[128];
        snprintf(topic, sizeof(topic), "%sota/status", _mqtt_ota_header);
        _mqtt_client->publish(topic, payload);
    }

    void _start_update()
    {
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN(F("Starting OTA update"));
#endif

        int comparison = _compareToCurrent(_new_version);
        if (comparison == 0)
        {
            // Normal after an update: the retained request is replayed on
            // reconnect until the server clears it. Nothing to report.
#ifdef DEBUG
            IOTCLOUD_LOG_PRINTLN(F("The device has already been updated. Ignore update"));
#endif
            return;
        }
        if (comparison < 0)
        {
            _publishOtaStatus(409, "Requested version is older than the running firmware");
            return;
        }
#ifdef ARDUINO_ARCH_ESP32
        // Don't reinstall an image that already failed here; a newer version
        // goes through. 410 also tells the server to drop a retained request.
        if (_rolledBackFrom(_new_version))
        {
            _publishOtaStatus(410, "This version already rolled back on this device");
            return;
        }
#endif

#ifdef ESP8266
        // Every attempt that returns clears the marker below, so one still set
        // for this version means the last attempt crashed.
        const uint8_t crashes = _crashesFor(_new_version);
        if (crashes >= OTA_MAX_CRASHES)
        {
            _publishOtaStatus(410, "This version crashed the device during OTA");
            return;
        }
        _markAttempt(_new_version, crashes + 1);
#endif

#ifdef ESP8266
        WiFiClientSecure client;
        X509List cert(IOTCLOUD_OTA_ROOT_CA);
        client.setTrustAnchors(&cert);
        // BearSSL's default 16 KB receive buffer doesn't fit next to the
        // sensors on busy devices: ~4 KB heap was left after it, so the
        // handshake failed or crashed with OOM. With a buffer under 16 KB,
        // BearSSL asks for TLS max fragment length in its own handshake, which
        // ota.iotcloud.es supports (~20 KB heap left). Don't gate this on
        // probeMaxFragmentLength(): its hand-built ClientHello has no SNI, and
        // the server rejects handshakes without a host name ("unrecognized
        // name", alert 112), so it always returned false. A server without
        // MFLN would fail the handshake cleanly (400), not crash.
        client.setBufferSizes(1024, 512);
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
#ifdef ESP8266
            _clearAttempt();
#endif
            _publishOtaStatus(400, buf);
            return;
        }

        char url[256];
        snprintf(url, sizeof(url), "https://%s/%s/%s", IotCloud_Constants::OTA_SERVER, _device_model, _new_version);

        // The download blocks MQTT until it ends.
        const unsigned long download_start = millis();
#ifdef ESP8266
        ESPhttpUpdate.setLedPin(IOTCLOUD_LED_PIN, LOW);
        t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
        _clearAttempt(); // It returned, so it didn't crash, whatever the result.
#elif defined(ARDUINO_ARCH_ESP32)
        httpUpdate.setLedPin(IOTCLOUD_LED_PIN, LOW);
        t_httpUpdate_return ret = httpUpdate.update(client, url);
#endif
        _reconnectAfterLongDownload(millis() - download_start);

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
            IOTCLOUD_LOG_PRINTLN(error_msg);
#endif
            _publishOtaStatus(400, error_msg);
            break;

        case HTTP_UPDATE_NO_UPDATES:
            strlcpy(error_msg, "HTTP_UPDATE_NO_UPDATES", sizeof(error_msg));
#ifdef DEBUG
            IOTCLOUD_LOG_PRINTLN(error_msg);
#endif
            _publishOtaStatus(400, error_msg);
            break;

        case HTTP_UPDATE_OK:
            strlcpy(error_msg, "Update finished successfully. Restarting...", sizeof(error_msg));
#ifdef DEBUG
            IOTCLOUD_LOG_PRINTLN(error_msg);
#endif
            _publishOtaStatus(200, error_msg);
#ifdef ARDUINO_ARCH_ESP32
            _setInstallingVersion(_new_version);
#endif
            // Give the network stack time to send the status message before restarting
            _mqtt_client->loop();
            delay(500);
            ESP.restart();
            break;
        }
    }

#ifdef ARDUINO_ARCH_ESP32
    void set_health_check(bool (*check)())
    {
        _health_check = check;
    }

    // Confirms a freshly installed image once MQTT has stayed up for
    // IOTCLOUD_OTA_CONFIRM_AFTER_MS and the project's check passes. Rolls back
    // if that hasn't happened by IOTCLOUD_OTA_CONFIRM_DEADLINE_MS after boot.
    // Does nothing unless the bootloader marked the image pending, which needs
    // a bootloader with rollback support (Arduino core 3.x).
    void _check_new_image()
    {
        if (_image_state == ImageState::Settled)
            return;
        if (_image_state == ImageState::Unknown)
        {
            esp_ota_img_states_t state;
            const bool pending =
                esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
                state == ESP_OTA_IMG_PENDING_VERIFY;
            _image_state = pending ? ImageState::Pending : ImageState::Settled;
            if (!pending)
                return;
            IOTCLOUD_LOG_PRINTLN(F("OTA: new image, waiting until it is healthy"));
        }

        const unsigned long now = millis();
        if (_mqtt_client == nullptr || !_mqtt_client->isConnected())
            _mqtt_up_since_ms = 0;
        else if (_mqtt_up_since_ms == 0)
            _mqtt_up_since_ms = now == 0 ? 1 : now;

        const bool healthy = _mqtt_up_since_ms != 0 &&
                             now - _mqtt_up_since_ms >= IOTCLOUD_OTA_CONFIRM_AFTER_MS &&
                             (_health_check == nullptr || _health_check());
        if (healthy)
        {
            _image_state = ImageState::Settled;
            esp_ota_mark_app_valid_cancel_rollback();
            _setInstallingVersion(nullptr);
            IOTCLOUD_LOG_PRINTLN(F("OTA: new image confirmed"));
            // The real success signal: installed, rebooted and healthy, sent
            // on a fresh connection. It also makes the server clear the
            // retained request if the 200 sent before the reboot was lost.
            if (_mqtt_ota_header != nullptr && _current_version != nullptr)
            {
                strlcpy(_new_version, _current_version, sizeof _new_version);
                _publishOtaStatus(200, "New image confirmed");
            }
        }
        else if (now >= IOTCLOUD_OTA_CONFIRM_DEADLINE_MS)
        {
            _image_state = ImageState::Settled; // If the rollback can't happen, stop trying.
            IOTCLOUD_LOG_PRINTLN(F("OTA: new image never got healthy, rolling back"));
            // Online but unhealthy (e.g. the project's health check failed):
            // say so. Offline, the server notices the old version coming back.
            if (_mqtt_ota_header != nullptr && _current_version != nullptr &&
                _mqtt_client != nullptr && _mqtt_client->isConnected())
            {
                strlcpy(_new_version, _current_version, sizeof _new_version);
                _publishOtaStatus(500, "New image failed its health check, rolling back");
                _mqtt_client->loop();
                delay(500);
            }
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
    }
#endif

    void loop()
    {
#ifdef ARDUINO_ARCH_ESP32
        _check_new_image();
#endif
        if (IotCloud_Constants::ENABLE_OTA && _ota_requested)
        {
            _start_update();
            _ota_requested = false;
        }
    }
} // namespace OTA
