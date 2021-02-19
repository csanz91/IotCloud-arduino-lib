#include "web_server.h"

ESP8266WebServer server(80);
Device *device;

#define DEBUG

unsigned long setupTime = 0;

void respond(bool result, const char *response_data)
{
    char responseBuffer[1256];
    sprintf(responseBuffer, "{\"result\": %s, \"data\": %s}", result ? "true" : "false", response_data);
    server.send(200, "application/json", responseBuffer);
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
        sprintf(errorBuffer, "deserializeJson() failed with code: %s", err.c_str());
        Serial.println(errorBuffer);
        respond(false, errorBuffer);
        return;
    }

    device_data data;
    if (strlcpy(data.deviceId, received_device_data["deviceId"], DEVICEID_MAX_SIZE) >= DEVICEID_MAX_SIZE)
    {
        respond(false, "DeviceId lenght is too big");
        return;
    }
    if (strlcpy(data.locationId, received_device_data["locationId"], LOCATIONID_MAX_SIZE) >= LOCATIONID_MAX_SIZE)
    {
        respond(false, "locationId lenght is too big");
        return;
    }
    if (strlcpy(data.token, received_device_data["token"], TOKEN_MAX_SIZE) >= TOKEN_MAX_SIZE)
    {
        respond(false, "token lenght is too big");
        return;
    }
    if (strlcpy(data.wifiSSID, received_device_data["wifiSSID"], WIFI_SSID_MAX_SIZE) >= WIFI_SSID_MAX_SIZE)
    {
        respond(false, "wifiSSID lenght is too big");
        return;
    }
    if (strlcpy(data.wifiPassword, received_device_data["wifiPassword"], WIFI_PASSWORD_MAX_SIZE) >= WIFI_PASSWORD_MAX_SIZE)
    {
        respond(false, "wifiPassword lenght is too big");
        return;
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
    respond(false, "Data is not valid");
}

void web_server_setup(Device *d)
{

    device = d;

    Serial.println("Starting web server...");
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
        Serial.println("Restarting device...");
        ESP.restart();
    }
}