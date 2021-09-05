
#include "setup_mode.h"

void config_mode_setup(Device *device)
{
    Serial.println("Setup mode started...");

    pinMode(LED_BUILTIN, OUTPUT);

    // Start access point
    Serial.println("Starting WiFi AP...");
    WiFi.persistent(false);
    WiFi.softAP(IotCloud_Constants::AP_SSID, IotCloud_Constants::AP_PASSWORD);

    web_server_setup(device);
}

void config_mode_loop()
{
    // Flash the LED when in config mode
    digitalWrite(LED_BUILTIN, millis() / LED_PIN_PERIOD % 2);
    web_server_loop();
}