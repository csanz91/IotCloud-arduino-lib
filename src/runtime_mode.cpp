#include "runtime_mode.h"

// Not static: some device sketches use it via "extern EspMQTTClient *mqtt_client;"
EspMQTTClient *mqtt_client;

// static: keep these generic names out of the global namespace so they don't
// clash with names in the user's sketch (e.g. a global "Device device")
static device_data data;
static Device *_device;

namespace
{
unsigned long offline_restart_timeout_ms = 0;
unsigned long offline_since_ms = 0;
bool offline_timer_active = false;

// A Wi-Fi change received over MQTT that hasn't reached the broker yet. The
// previous credentials are kept in pending_fallback until then.
bool network_change_pending = false;
network_fallback pending_fallback;
// Currently trying the previous credentials instead of the new ones
bool using_fallback_network = false;
unsigned long network_attempt_since_ms = 0;

const char *active_wifi_ssid()
{
    return using_fallback_network ? pending_fallback.wifiSSID : data.wifiSSID;
}

const char *active_wifi_password()
{
    return using_fallback_network ? pending_fallback.wifiPassword : data.wifiPassword;
}

// How long to try one set of credentials before switching to the other. Kept
// below the offline restart timeout so both sets get tried before a restart.
unsigned long network_attempt_timeout_ms()
{
    if (offline_restart_timeout_ms > 0 && offline_restart_timeout_ms / 2 < IOTCLOUD_NETWORK_FALLBACK_TIMEOUT_MS)
        return offline_restart_timeout_ms / 2;
    return IOTCLOUD_NETWORK_FALLBACK_TIMEOUT_MS;
}

// While a Wi-Fi change is pending, switch between the new and previous
// credentials whenever one set can't reach the broker for a while. This
// happens in place, without restarting, so local state is kept.
void handle_pending_network_change()
{
    if (!network_change_pending)
        return;

    unsigned long now = millis();
    if (mqtt_client->isConnected())
    {
        network_attempt_since_ms = now;
        return;
    }
    if (now - network_attempt_since_ms < network_attempt_timeout_ms())
        return;

    using_fallback_network = !using_fallback_network;
    network_attempt_since_ms = now;
    IOTCLOUD_LOG_PRINTLN(using_fallback_network ? F("Wi-Fi change pending: trying previous credentials")
                                                : F("Wi-Fi change pending: trying new credentials"));
    mqtt_client->setWifiCredentials(active_wifi_ssid(), active_wifi_password());
    // EspMQTTClient reconnects with the new credentials once it sees the link drop
    WiFi.disconnect();
}
}

void runtime_mode_setup(const device_data *received_data, Device *d, unsigned long restart_timeout_ms)
{
    data = *received_data;
    _device = d;

    if (get_network_fallback(&pending_fallback))
    {
        // Start with the new credentials. handle_pending_network_change()
        // switches to the previous ones if they don't connect.
        network_change_pending = true;
        network_attempt_since_ms = millis();
        // Otherwise the SDK saves every credential switch to flash
        WiFi.persistent(false);
        IOTCLOUD_LOG_PRINTLN(F("Wi-Fi change pending: trying new credentials"));
    }

    offline_restart_timeout_ms = restart_timeout_ms;
    offline_since_ms = millis();
    offline_timer_active = restart_timeout_ms > 0;
    snprintf(d->mqtt_header, sizeof(d->mqtt_header), "v1/%s/%s/", data.locationId, data.deviceId);
    _device->get_topic(d->mqtt_status_topic, sizeof(d->mqtt_status_topic), "status");

    mqtt_client = new EspMQTTClient(
        data.wifiSSID,
        data.wifiPassword,
        IotCloud_Constants::MQTT_SERVER,
        data.token,    // Username
        "_",           // Password
        data.deviceId, // Client ID
        1883);

    mqtt_client->setMaxPacketSize(1024);

#if defined(DEBUG) && !defined(IOTCLOUD_DISABLE_SERIAL_LOGGING)
    mqtt_client->enableDebuggingMessages();
#endif
    mqtt_client->enableLastWillMessage(_device->mqtt_status_topic, "offline", true);
}

void onConnectionEstablished()
{
#ifdef DEBUG
    IOTCLOUD_LOG_PRINTLN(F("Connected to the mqtt server"));
#endif
    configTzTime(TZ_Europe_Madrid, IotCloud_Constants::NTP_SERVER);

    if (network_change_pending && !using_fallback_network)
    {
        // The new Wi-Fi credentials work, so the previous ones aren't needed
        clear_network_fallback();
        network_change_pending = false;
        IOTCLOUD_LOG_PRINTLN(F("Wi-Fi change confirmed"));
    }

    _device->init(mqtt_client);
    OTA::init(_device->mqtt_header, _device->device_current_version, _device->device_model, mqtt_client);
}

void runtime_mode_loop()
{
    mqtt_client->loop();
    _device->loop();
    OTA::loop();
    handle_pending_network_change();

    if (offline_restart_timeout_ms == 0)
    {
        return;
    }

    if (mqtt_client->isConnected())
    {
        offline_timer_active = false;
        return;
    }

    unsigned long now = millis();
    if (!offline_timer_active)
    {
        offline_since_ms = now;
        offline_timer_active = true;
        return;
    }

    if (now - offline_since_ms >= offline_restart_timeout_ms)
    {
        IOTCLOUD_LOG_PRINTLN(F("IoTCloud connectivity timeout; restarting device..."));
        IOTCLOUD_LOG_FLUSH();
        ESP.restart();
    }
}
void runtime_mode_network_update(const char *ssid, const char *password)
{
    device_data saved = get_data();

    // Ignore the credentials already saved, e.g. a retained message that is
    // delivered again on every connect.
    if (strcmp(saved.wifiSSID, ssid) == 0 && strcmp(saved.wifiPassword, password) == 0)
    {
        return;
    }

    device_data updated = saved;
    if (strlcpy(updated.wifiSSID, ssid, WIFI_SSID_MAX_SIZE) >= WIFI_SSID_MAX_SIZE ||
        strlcpy(updated.wifiPassword, password, WIFI_PASSWORD_MAX_SIZE) >= WIFI_PASSWORD_MAX_SIZE)
    {
        IOTCLOUD_LOG_PRINTLN(F("Wi-Fi change rejected: SSID or password too long"));
        return;
    }
    if (!validate_device_data(&updated))
    {
        IOTCLOUD_LOG_PRINTLN(F("Wi-Fi change rejected: invalid data"));
        return;
    }

    // The credentials in use right now work (this message arrived over them),
    // so receiving them again cancels a pending change without a restart.
    if (strcmp(active_wifi_ssid(), ssid) == 0 && strcmp(active_wifi_password(), password) == 0)
    {
        save_data(&updated);
        clear_network_fallback();
        strlcpy(data.wifiSSID, ssid, sizeof(data.wifiSSID));
        strlcpy(data.wifiPassword, password, sizeof(data.wifiPassword));
        network_change_pending = false;
        using_fallback_network = false;
        mqtt_client->setWifiCredentials(data.wifiSSID, data.wifiPassword);
        return;
    }

    // Keep the working credentials as a fallback before switching, in case the
    // new ones never connect. Saved first so a power cut in between is harmless.
    network_fallback fallback;
    strlcpy(fallback.wifiSSID, active_wifi_ssid(), sizeof(fallback.wifiSSID));
    strlcpy(fallback.wifiPassword, active_wifi_password(), sizeof(fallback.wifiPassword));
    save_network_fallback(&fallback);
    save_data(&updated);

    IOTCLOUD_LOG_PRINTLN(F("Wi-Fi change saved; restarting..."));
    IOTCLOUD_LOG_FLUSH();
    ESP.restart();
}
