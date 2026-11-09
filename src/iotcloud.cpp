#include <Arduino.h>
#include "iotcloud.h"

//#define DEBUG

// static: keep these generic names out of the global namespace
static int currentState;
static unsigned long releasedTime = 0;

static bool runtime_mode = false;

void iotcloud_setup(Device *device, unsigned long offline_restart_timeout_ms)
{
    // Make sure the device passed is correct
    assert(device);

    IotCloud_Constants::setup_constants();

    // Set up the reset button pin
    pinMode(IOTCLOUD_RESET_PIN, IOTCLOUD_RESET_PIN_MODE);

    // Get the device data saved
    device_data data = get_data();
    // If the saved data is valid, activate the runtime mode
    runtime_mode = validate_device_data(&data);
#ifdef DEBUG
    pretty_print_data(&data);
#endif

    if (runtime_mode)
    {
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN(F("Starting runtime mode setup"));
#endif
        runtime_mode_setup(&data, device, offline_restart_timeout_ms);
    }
    else
    {
#ifdef DEBUG
        IOTCLOUD_LOG_PRINTLN(F("Starting config mode setup"));
#endif
        config_mode_setup(device);
    }
}


void iotcloud_loop(void)
{
    if (runtime_mode)
    {
        runtime_mode_loop();
    }
    else
    {
        config_mode_loop();
    }
    detect_reset_request();

}

void detect_reset_request()
{
    currentState = digitalRead(IOTCLOUD_RESET_PIN);

    if (currentState == HIGH) // button is not pressed
        releasedTime = millis();

    unsigned long pressDuration = millis() - releasedTime;

    if (pressDuration > RESET_PRESS_TIME)
    {
        IOTCLOUD_LOG_PRINTLN(F("Resetting device..."));
        clear_saved_data();
        IOTCLOUD_LOG_PRINTLN(F("Restarting device..."));
        ESP.restart();
    }

}
