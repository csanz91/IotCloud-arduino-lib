#include <Arduino.h>
#include "iotcloud.h"

//#define DEBUG

int currentState;
unsigned long releasedTime = 0;

bool runtime_mode = false;

void iotcloud_setup(Device *device)
{
    // Make sure the device passed is correct
    assert(device);

    IotCloud_Constants::setup_constants();

    // Set up the reset button pin
    pinMode(RESET_PIN, INPUT);

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
        Serial.println(F("Starting runtime mode setup"));
#endif
        runtime_mode_setup(&data, device);
    }
    else
    {
#ifdef DEBUG
        Serial.println(F("Starting config mode setup"));
#endif
        config_mode_setup(device);
    }
}

#ifdef DEBUG
unsigned int lastDebugMessage = millis();
#endif

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

#ifdef DEBUG
    unsigned int elapsed = millis() - lastDebugMessage;
    if (elapsed > 60000)
    {
        Serial.print(F("Heap fragmentation: "));
        Serial.println(ESP.getHeapFragmentation());
        Serial.print(F("Heap free: "));
        Serial.println(ESP.getFreeHeap());
        Serial.println();

        lastDebugMessage = millis();
    }

#endif
}

void detect_reset_request()
{
    currentState = digitalRead(RESET_PIN);

    if (currentState == HIGH) // button is not pressed
        releasedTime = millis();

    long pressDuration = millis() - releasedTime;

    if (pressDuration > RESET_PRESS_TIME)
    {
        Serial.println(F("Resetting device..."));
        clear_saved_data();
        Serial.println(F("Restarting device..."));
        ESP.restart();
    }

}