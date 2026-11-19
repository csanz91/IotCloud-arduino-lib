# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

ESP8266/ESP32 IoT firmware library for the IotCloud home automation platform. Devices connect via MQTT to `mqtt.iotcloud.es`. There is no build step or test suite in this repo — consumer projects pull it as an Arduino/PlatformIO dependency and compile for their target board.

## Version Bumping

The library version lives in two files that **must** be updated together on every release:

- `library.properties` line 2: `version=X.Y.Z`
- `src/iotcloud_constants.cpp`: `const char IotCloud_Constants::LIB_VERSION[] = "X.Y.Z";`

If you change one, change the other in the same commit.

## Architecture

### Two-Mode Lifecycle

The firmware has two mutually exclusive modes, chosen at boot based on whether valid config exists in EEPROM:

1. **Setup Mode** (`src/setup/`): No config → starts a WiFi AP (`iot_XXXX` / `12345678`) with a captive-portal web server. Two endpoints: `GET /getDeviceInfo` (returns device JSON) and `POST /deviceSetup` (saves config to EEPROM, restarts into runtime mode).

2. **Runtime Mode** (`src/runtime_mode.cpp`): Valid config → connects to WiFi, creates `EspMQTTClient`, initializes device and sensors on MQTT connect. The `onConnectionEstablished()` function in `runtime_mode.cpp` is a **global callback required by the EspMQTTClient library** — it's not a method on any class; the library calls it by name.

Entry point: `iotcloud_setup(Device*)` reads EEPROM, validates, and branches. `iotcloud_loop()` delegates to the active mode. Both modes run `detect_reset_request()` (GPIO 0 held 5s → EEPROM clear → restart).

### MQTT Topic Construction

All topics use the pattern `v1/{locationId}/{deviceId}/`. This prefix is stored in `Device::mqtt_header` and built once at runtime startup. Sensor topics append `{sensorId}/{endpoint}`. Sensor IDs are auto-prefixed with the device internal ID (MAC-derived): `{macPrefix}_{userSensorId}`.

### Sensor Hierarchy

```
BaseSensor (sensor.h)
├── AnalogSensor (analog.h) — periodic value reporting, running-average filter, offset calibration
│   └── Thermostat (thermostat.h) — adds state and heating control
├── SwitchSensor (switch.h) — binary on/off state
│   └── LedSensor (led.h) — adds brightness
│       └── RgbSensor (rgb.h) — adds color
└── Toggle (toggle.h) — tri-state (up/down/toggle) with timed actions
```

To create a new sensor type: subclass the closest base, override `init()` to subscribe to MQTT topics, and override the relevant virtual methods (`get_value()`, `set_value()`, `set_state()`, etc.). Call `construct_topic()` to build topic strings. The sensor's `init()` receives `mqtt_header` and the `EspMQTTClient*` for subscriptions/publications.

### Platform Conditionals

Code uses `#ifdef ESP8266` / `#elif defined(ARDUINO_ARCH_ESP32)` throughout (WiFi, HTTP update, web server, TLS). ESP32-C3 gets special handling for `RESET_PIN` (GPIO 9 instead of 0). When touching platform-specific code, update both branches.

### EEPROM Persistence

`device_data` struct is stored raw in EEPROM at address 0. `get_data()` attempts to read the current format first; if invalid, tries a legacy format (smaller token field, no subsensor fields) and migrates in-place. Optional fields (`subsensorId`, `subdeviceId`) are sanitized on read since they may contain EEPROM garbage from older firmware.

### Compile-Time Configuration

`src/IotCloud_config.h` provides:
- `IOTCLOUD_DISABLE_SERIAL_LOGGING`, reset/LED pin overrides, and timeouts (`IOTCLOUD_NETWORK_FALLBACK_TIMEOUT_MS`, `IOTCLOUD_THERMOSTAT_OFFLINE_HEATING_TIMEOUT_MS`)
- `IOTCLOUD_SENSOR_EXPORT_JSON_SIZE` / `IOTCLOUD_DEVICE_EXPORT_JSON_SIZE` to resize JSON export buffers (increase when devices have large metadata)

Consumer projects must set these as build flags (`-DNAME=value`); a `#define` in the sketch does not reach the library's separately compiled `.cpp` files. The IotCloud-sensors repo uses opt-in `IOTCLOUD_ENABLE_*` flags the same way.

### OTA Updates

`src/ota.cpp` subscribes to `v1/ota/update/{model}`, compares versions, downloads firmware over HTTPS from `ota.iotcloud.es` with certificate pinning (`src/certs/iotcloud_ota_pem.h`). Disable with `IotCloud_Constants::ENABLE_OTA = false`.

### Debug Mode

Building with `-DDEBUG` (a build flag, not a sketch `#define`) enables Serial output for config data, MQTT connections, OTA progress, and sensor changes. The define is checked via `#ifdef DEBUG` throughout.

## Key Paths

- `src/iotcloud.h` / `src/iotcloud.cpp` — entry point (`iotcloud_setup`, `iotcloud_loop`, reset detection)
- `src/device.h` / `src/device.cpp` — Device class, MQTT init, JSON export, sensor management
- `src/device_data.h` / `src/device_data.cpp` — EEPROM struct, read/write/validate, legacy migration
- `src/runtime_mode.cpp` — WiFi+MQTT client setup, `onConnectionEstablished` callback, NTP time sync
- `src/setup/` — captive portal web server for first-time device configuration
- `src/iotcloud_constants.h` / `src/iotcloud_constants.cpp` — servers, version, MAC-derived device ID
- `src/IotCloud_config.h` — compile-time feature toggles and buffer sizes
- `src/sensors/` — sensor implementations (see hierarchy above)
- `src/ota.cpp` — OTA update handler with HTTPS + cert pinning
- `src/certs/iotcloud_ota_pem.h` — root CA certificate for OTA server

## Dependencies

- [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient) — MQTT + WiFi management (requires the global `onConnectionEstablished()` callback)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) — JSON serialization (uses v6 API: `DynamicJsonDocument`, `StaticJsonDocument`)
- Platform SDK WiFi, HTTP update, EEPROM, WebServer libraries
