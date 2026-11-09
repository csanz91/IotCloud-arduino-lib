
#pragma once

// Configuration file for IotCloud libraries.
//
// These options are read by the library's own source files, which are
// compiled separately from your sketch. A #define in the sketch does NOT reach
// them: set options as build flags instead (PlatformIO: build_flags = -DNAME=value),
// or edit this file (Arduino IDE).

// --- Logging ---
// Define this to remove all serial logging emitted by the IotCloud library.
// #define IOTCLOUD_DISABLE_SERIAL_LOGGING

#ifdef IOTCLOUD_DISABLE_SERIAL_LOGGING
#define IOTCLOUD_LOG_PRINT(...) do { } while (0)
#define IOTCLOUD_LOG_PRINTLN(...) do { } while (0)
#define IOTCLOUD_LOG_FLUSH() do { } while (0)
#else
#define IOTCLOUD_LOG_PRINT(...) do { Serial.print(__VA_ARGS__); } while (0)
#define IOTCLOUD_LOG_PRINTLN(...) do { Serial.println(__VA_ARGS__); } while (0)
#define IOTCLOUD_LOG_FLUSH() do { Serial.flush(); } while (0)
#endif

// --- Sensors ---
// This library's sensors need no flag: sensors a device doesn't use are left
// out at link time. Sensors in IotCloud-sensors are opt-in with build flags
// such as -DIOTCLOUD_ENABLE_DS18B20 (see that repo's README).

// --- GPIO pin overrides ---
// Override the default reset button and status LED pins per device.
// #define IOTCLOUD_RESET_PIN 4
// #define IOTCLOUD_LED_PIN 2
// #define IOTCLOUD_RESET_PIN_MODE INPUT_PULLUP
#ifndef IOTCLOUD_RESET_PIN_MODE
#define IOTCLOUD_RESET_PIN_MODE INPUT
#endif

#ifndef IOTCLOUD_LED_PIN
#define IOTCLOUD_LED_PIN LED_BUILTIN
#endif

// --- Remote Wi-Fi change ---
// After a Wi-Fi change received over MQTT, the device keeps the previous
// credentials and switches between the new and previous ones (without
// restarting) until the new ones reach the broker. This is how long it tries
// one set before switching to the other.
#ifndef IOTCLOUD_NETWORK_FALLBACK_TIMEOUT_MS
#define IOTCLOUD_NETWORK_FALLBACK_TIMEOUT_MS 180000UL
#endif

// --- Thermostat ---
// Turn heating off when the thermostat has been offline this long while
// heating, since only the cloud can turn it off otherwise. 0 disables it.
#ifndef IOTCLOUD_THERMOSTAT_OFFLINE_HEATING_TIMEOUT_MS
#define IOTCLOUD_THERMOSTAT_OFFLINE_HEATING_TIMEOUT_MS 1800000UL // 30 minutes
#endif

// --- JSON/export buffer sizes ---
// Increase these when devices need larger sensor metadata payloads.
// Larger values consume more RAM while setup mode exports device info.
#ifndef IOTCLOUD_SENSOR_EXPORT_JSON_SIZE
#define IOTCLOUD_SENSOR_EXPORT_JSON_SIZE 1024
#endif

#ifndef IOTCLOUD_DEVICE_EXPORT_JSON_SIZE
#define IOTCLOUD_DEVICE_EXPORT_JSON_SIZE 4096
#endif
