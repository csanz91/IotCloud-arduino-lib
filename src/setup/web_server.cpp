#include "web_server.h"
#include "../IotCloud_config.h"
#include <stdlib.h>

// static: keep these generic names out of the global namespace so they don't
// clash with names in the user's sketch (e.g. a global "Device device")
static Device *device;
#ifdef ESP8266
#include <ESP8266WebServer.h>
static ESP8266WebServer server(80);
#elif defined(ARDUINO_ARCH_ESP32)
#include <WebServer.h>
static WebServer server(80);
#endif

static unsigned long setupTime = 0;

static void respond(bool result, const char *response_data)
{
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent(result ? "{\"result\": true, \"data\": " : "{\"result\": false, \"data\": ");
    server.sendContent(response_data);
    server.sendContent("}");
}

static void respond_message(bool result, const char *message)
{
    StaticJsonDocument<256> response;
    response[F("result")] = result;
    response[F("data")] = message ? message : "";

    char payload[384];
    size_t payload_size = serializeJson(response, payload, sizeof(payload));
    if (payload_size == 0 || payload_size >= sizeof(payload))
    {
        // Keep the error response valid JSON even if the diagnostic message
        // is unexpectedly long.
        server.send(200, "application/json", R"({"result":false,"data":"setup error"})");
        return;
    }

    server.send(200, "application/json", payload);
}

static void get_device_info()
{
    char *response = static_cast<char *>(malloc(IOTCLOUD_DEVICE_EXPORT_JSON_SIZE));
    if (!response)
    {
        respond_message(false, "Not enough memory to export device info");
        return;
    }

    if (!device->export_data(response, IOTCLOUD_DEVICE_EXPORT_JSON_SIZE))
    {
        free(response);
        respond_message(false, "Device info is too large");
        return;
    }

    respond(true, response);
    free(response);
}

static void device_setup()
{
    // The current app includes a JWT plus the optional deviceInternalId in
    // this request. 512 bytes is not enough once ArduinoJson stores the
    // strings, so deserialization would fail and the old error response was
    // itself malformed JSON.
    StaticJsonDocument<1024> received_device_data;
    DeserializationError err = deserializeJson(received_device_data, server.arg("plain"));
    if (err)
    {
        char errorBuffer[64];
        snprintf(errorBuffer, sizeof(errorBuffer), "deserializeJson() failed with code: %s", err.c_str());
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN(errorBuffer);
#endif
        respond_message(false, errorBuffer);
        return;
    }

    // ArduinoJson returns nullptr for a missing or non-string field, and
    // strlcpy() would crash on it, so check every required field first.
    const char *required_fields[] = {"deviceId", "locationId", "token", "wifiSSID", "wifiPassword"};
    for (const char *field : required_fields)
    {
        if (!received_device_data[field].is<const char *>())
        {
            char message[64];
            snprintf(message, sizeof(message), "%s is missing or not a string", field);
            respond_message(false, message);
            return;
        }
    }

    device_data data;
    if (strlcpy(data.deviceId, received_device_data["deviceId"], DEVICEID_MAX_SIZE) >= DEVICEID_MAX_SIZE)
    {
        respond_message(false, "DeviceId length is too big");
        return;
    }
    if (strlcpy(data.locationId, received_device_data["locationId"], LOCATIONID_MAX_SIZE) >= LOCATIONID_MAX_SIZE)
    {
        respond_message(false, "locationId length is too big");
        return;
    }
    if (strlcpy(data.token, received_device_data["token"], TOKEN_MAX_SIZE) >= TOKEN_MAX_SIZE)
    {
        respond_message(false, "token length is too big");
        return;
    }
    if (strlcpy(data.wifiSSID, received_device_data["wifiSSID"], WIFI_SSID_MAX_SIZE) >= WIFI_SSID_MAX_SIZE)
    {
        respond_message(false, "wifiSSID length is too big");
        return;
    }
    if (strlcpy(data.wifiPassword, received_device_data["wifiPassword"], WIFI_PASSWORD_MAX_SIZE) >= WIFI_PASSWORD_MAX_SIZE)
    {
        respond_message(false, "wifiPassword length is too big");
        return;
    }
    // Optional fields (subsensor devices): a missing, null or non-string value means "not set"
    if (received_device_data["subsensorId"].is<const char *>())
    {
        if (strlcpy(data.subsensorId, received_device_data["subsensorId"], SUBSENSORID_MAX_SIZE) >= SUBSENSORID_MAX_SIZE)
        {
            respond_message(false, "subsensorId length is too big");
            return;
        }
    }
    if (received_device_data["subdeviceId"].is<const char *>())
    {
        if (strlcpy(data.subdeviceId, received_device_data["subdeviceId"], SUBDEVICEID_MAX_SIZE) >= SUBDEVICEID_MAX_SIZE)
        {
            respond_message(false, "subdeviceId length is too big");
            return;
        }
    }

#ifdef DEBUG
    pretty_print_data(&data);
#endif

    if (validate_device_data(&data))
    {
        save_data(&data);
        clear_network_fallback();
        respond_message(true, "ok");
        setupTime = millis();
    }
    else
    {
        respond_message(false, "Data is not valid");
    }
}

void web_server_setup(Device *d)
{

    device = d;
#ifdef DEBUG
    IOTCLOUD_LOG_PRINTLN("Starting web server...");
#endif
    // Define endpoints and its handlers
    server.on("/getDeviceInfo", HTTP_GET, get_device_info);
    server.on("/deviceSetup", HTTP_POST, device_setup);

    // Start web server
    server.begin();
}

void web_server_loop()
{
    server.handleClient();

    // Let response and close the connection with the web clients before restaring the device
    if (setupTime > 0 && (millis() - setupTime) > 5000)
    {
        server.close();
        server.stop();
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN("Restarting device...");
#endif
        ESP.restart();
    }
}
