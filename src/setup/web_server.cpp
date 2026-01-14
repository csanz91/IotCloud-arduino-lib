#include "web_server.h"

Device *device;
#ifdef ESP8266
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#elif defined(ARDUINO_ARCH_ESP32)
#include <WebServer.h>
WebServer server(80);
#endif

unsigned long setupTime = 0;

void respond(bool result, const char *response_data)
{
    char responseBuffer[1256];
    snprintf(responseBuffer, sizeof(responseBuffer), "{\"result\": %s, \"data\": %s}", result ? "true" : "false", response_data);
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent(responseBuffer);
}

void get_device_info()
{
    char response[1024];
    device->export_data(response);

    respond(true, response);
}

void device_setup()
{
    StaticJsonDocument<512> received_device_data;
    DeserializationError err = deserializeJson(received_device_data, server.arg("plain"));
    if (err)
    {
        char errorBuffer[64];
        snprintf(errorBuffer, sizeof(errorBuffer), "deserializeJson() failed with code: %s", err.c_str());
#ifdef DEBUG
        Serial.println(errorBuffer);
#endif
        respond(false, errorBuffer);
        return;
    }

    device_data data;
    if (strlcpy(data.deviceId, received_device_data["deviceId"], DEVICEID_MAX_SIZE) >= DEVICEID_MAX_SIZE)
    {
        respond(false, "DeviceId length is too big");
        return;
    }
    if (strlcpy(data.locationId, received_device_data["locationId"], LOCATIONID_MAX_SIZE) >= LOCATIONID_MAX_SIZE)
    {
        respond(false, "locationId length is too big");
        return;
    }
    if (strlcpy(data.token, received_device_data["token"], TOKEN_MAX_SIZE) >= TOKEN_MAX_SIZE)
    {
        respond(false, "token length is too big");
        return;
    }
    if (strlcpy(data.wifiSSID, received_device_data["wifiSSID"], WIFI_SSID_MAX_SIZE) >= WIFI_SSID_MAX_SIZE)
    {
        respond(false, "wifiSSID length is too big");
        return;
    }
    if (strlcpy(data.wifiPassword, received_device_data["wifiPassword"], WIFI_PASSWORD_MAX_SIZE) >= WIFI_PASSWORD_MAX_SIZE)
    {
        respond(false, "wifiPassword length is too big");
        return;
    }
    // Optional: subsensorId for subsensor devices
    if (received_device_data.containsKey("subsensorId"))
    {
        if (strlcpy(data.subsensorId, received_device_data["subsensorId"], SUBSENSORID_MAX_SIZE) >= SUBSENSORID_MAX_SIZE)
        {
            respond(false, "subsensorId length is too big");
            return;
        }
    }
    if (received_device_data.containsKey("subdeviceId"))
    {
        if (strlcpy(data.subdeviceId, received_device_data["subdeviceId"], SUBDEVICEID_MAX_SIZE) >= SUBDEVICEID_MAX_SIZE)
        {
            respond(false, "subdeviceId length is too big");
            return;
        }
    }

#ifdef DEBUG
    pretty_print_data(&data);
#endif

    if (validate_device_data(&data))
    {
        save_data(&data);
        respond(true, "\"ok\"");
        setupTime = millis();
    }
    else
    {
        respond(false, "Data is not valid");
    }
}

void web_server_setup(Device *d)
{

    device = d;
#ifdef DEBUG
    Serial.println("Starting web server...");
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
        Serial.println("Restarting device...");
#endif
        ESP.restart();
    }
}