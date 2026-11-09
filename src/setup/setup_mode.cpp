
#include "setup_mode.h"

void config_mode_setup(Device *device)
{
#ifdef DEBUG
    IOTCLOUD_LOG_PRINTLN("Setup mode started...");
#endif

    pinMode(IOTCLOUD_LED_PIN, OUTPUT);

    // Start access point
#ifdef DEBUG
    IOTCLOUD_LOG_PRINTLN("Starting WiFi AP...");
#endif
    WiFi.persistent(false);
    WiFi.softAP(IotCloud_Constants::AP_SSID, IotCloud_Constants::AP_PASSWORD);

    web_server_setup(device);
}

void config_mode_loop()
{
    // Flash the LED when in config mode
    digitalWrite(IOTCLOUD_LED_PIN, millis() / LED_PIN_PERIOD % 2);
    web_server_loop();
}
