# IotCloud Arduino Library

Arduino library for the IotCloud home automation platform. This library provides a complete firmware framework for ESP8266 and ESP32 devices to integrate with the IotCloud IoT ecosystem.

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](https://www.gnu.org/licenses/lgpl-2.1)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [API Reference](#api-reference)
  - [Core Functions](#core-functions)
  - [Device Class](#device-class)
  - [Sensor Types](#sensor-types)
- [Configuration Mode](#configuration-mode)
- [Runtime Mode](#runtime-mode)
- [OTA Updates](#ota-updates)
- [MQTT Topics](#mqtt-topics)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

The IotCloud Arduino Library is a complete IoT firmware solution that provides:

- **Automatic WiFi configuration** via captive portal (Setup Mode)
- **MQTT-based communication** with the IotCloud platform
- **Over-The-Air (OTA) updates** with secure HTTPS
- **Multiple sensor types** support (analog, switch, LED, RGB, thermostat, toggle)
- **Dual-platform support** for ESP8266 and ESP32 microcontrollers
- **Persistent device configuration** stored in EEPROM
- **Factory reset** via physical button (GPIO 0 / GPIO 9 for ESP32-C3)

## Features

### Device Management
- Unique device identification using MAC address
- Automatic status reporting (online/offline via Last Will and Testament)
- IP address, version, and model reporting
- Remote reset and restart capabilities

### Sensor Types
- **AnalogSensor**: Continuous value reporting with configurable filtering
- **SwitchSensor**: Binary on/off state control
- **LedSensor**: Dimmable light control with brightness memory
- **RgbSensor**: Full RGB color control with brightness
- **Thermostat**: Temperature control with heating state management
- **Toggle**: Tri-state toggle control (up/down/toggle)

### Connectivity
- WPA2 WiFi connectivity
- MQTT with authentication (username/token)
- TLS/SSL support for OTA updates
- Automatic reconnection handling

### Time Synchronization
- NTP time synchronization
- Timezone support (Europe/Madrid default)
- Comprehensive timezone database included

## Requirements

### Hardware
- ESP8266 or ESP32-based microcontroller
- GPIO 0 available for factory reset button (GPIO 9 for ESP32-C3)

### Dependencies
- [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- ESP8266WiFi (for ESP8266) or WiFi (for ESP32)
- ESP8266httpUpdate (for ESP8266) or HTTPUpdate (for ESP32)
- EEPROM library

### Platform Support
- ESP8266 boards
- ESP32 boards (including ESP32-C3)

## Installation

### Arduino Library Manager
1. Open Arduino IDE
2. Go to **Sketch** → **Include Library** → **Manage Libraries...**
3. Search for "IotCloud"
4. Click **Install**

### Manual Installation
1. Download the latest release from [GitHub](https://github.com/csanz91/IotCloud-arduino-lib)
2. Extract to your Arduino libraries folder:
   - Windows: `Documents\Arduino\libraries\`
   - macOS: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`
3. Restart the Arduino IDE

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps =
    https://github.com/csanz91/IotCloud-arduino-lib.git
```

## Quick Start

### Basic Example

```cpp
#include <iotcloud.h>
#include <sensors/switch.h>

// Create device with version and model
Device device("1.0.0", "my_device_model");

// Create a switch sensor
SwitchSensor mySensor("switch1", "Main Light");

void setup() {
    Serial.begin(115200);
    
    // Add sensors to device
    device.add_sensor(&mySensor);
    
    // Initialize IotCloud
    iotcloud_setup(&device);
}

void loop() {
    // Main IotCloud loop
    iotcloud_loop();
}
```

### Thermostat Example

```cpp
#include <iotcloud.h>
#include <sensors/thermostat.h>

Device device("1.0.0", "thermostat_v1");
Thermostat thermostat("temp1", "Living Room Thermostat");

void setup() {
    Serial.begin(115200);
    device.add_sensor(&thermostat);
    iotcloud_setup(&device);
}

void loop() {
    iotcloud_loop();
    // Your temperature reading logic here
}
```

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                        IotCloud Library                       │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────────┐    ┌─────────────────┐                  │
│  │   Setup Mode    │    │  Runtime Mode   │                  │
│  │                 │    │                 │                  │
│  │  ┌───────────┐  │    │  ┌───────────┐  │                  │
│  │  │ WiFi AP   │  │    │  │ WiFi STA  │  │                  │
│  │  └───────────┘  │    │  └───────────┘  │                  │
│  │  ┌───────────┐  │    │  ┌───────────┐  │                  │
│  │  │WebServer  │  │    │  │MQTT Client│  │                  │
│  │  └───────────┘  │    │  └───────────┘  │                  │
│  └─────────────────┘    │  ┌───────────┐  │                  │
│                         │  │OTA Handler│  │                  │
│                         │  └───────────┘  │                  │
│                         └─────────────────┘                  │
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │                      Device Manager                       │ │
│  │                                                           │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐    │ │
│  │  │ Analog   │ │ Switch   │ │   LED    │ │   RGB    │    │ │
│  │  │ Sensor   │ │ Sensor   │ │ Sensor   │ │ Sensor   │    │ │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘    │ │
│  │  ┌──────────┐ ┌──────────┐                               │ │
│  │  │Thermostat│ │  Toggle  │                               │ │
│  │  └──────────┘ └──────────┘                               │ │
│  └──────────────────────────────────────────────────────────┘ │
│                                                               │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │                     Persistent Storage                    │ │
│  │                        (EEPROM)                           │ │
│  └──────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

## API Reference

### Core Functions

#### `iotcloud_setup(Device *device)`
Initializes the IotCloud library with the given device configuration.

```cpp
void setup() {
    Device device("1.0.0", "my_model");
    iotcloud_setup(&device);
}
```

#### `iotcloud_loop()`
Main loop function. Must be called in your `loop()` function.

```cpp
void loop() {
    iotcloud_loop();
}
```

### Device Class

#### Constructor
```cpp
Device(const char *device_current_version, const char *device_model)
```

| Parameter | Description |
|-----------|-------------|
| `device_current_version` | Firmware version string (e.g., "1.0.0") |
| `device_model` | Device model identifier for OTA updates |

#### Methods

| Method | Description |
|--------|-------------|
| `add_sensor(BaseSensor *sensor)` | Add a sensor to the device |
| `init(EspMQTTClient *mqtt_client)` | Initialize device with MQTT client |
| `export_data(char *exported_data)` | Export device information as JSON |
| `get_topic(char *topic, const char *endpoint)` | Construct MQTT topic |
| `loop()` | Device loop (called automatically) |

### Sensor Types

#### BaseSensor (Abstract Base Class)

All sensors inherit from `BaseSensor`:

```cpp
BaseSensor(
    const char *sensor_id,      // Unique sensor identifier
    const char *sensor_name,    // Human-readable name
    const char *sensor_type,    // Sensor type string
    const char *metadata = "{}" // Additional JSON metadata
);
```

#### AnalogSensor

For continuous value sensors (temperature, humidity, etc.):

```cpp
AnalogSensor(
    const char *sensor_id,
    const char *sensor_name,
    const unsigned int report_value_period = 30000, // ms between reports
    const unsigned short decimals = 2,               // decimal places
    const bool enable_filter = true,                 // enable smoothing
    const char *sensor_type = "analog",
    const char *metadata = "{}",
    const float filter_rate_change = 0.1            // max rate of change
);
```

**Features:**
- Configurable reporting period
- Running average filter with rate-of-change limiting
- Offset calibration via MQTT
- NaN/Inf value filtering

**Subscribes to:**
- `{mqtt_header}{sensor_id}/aux/offset` - Set value offset

**Publishes to:**
- `{mqtt_header}{sensor_id}/value` - Current value

#### SwitchSensor

For binary on/off devices:

```cpp
SwitchSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type = "switch",
    const char *metadata = "{}"
);
```

**Subscribes to:**
- `{mqtt_header}{sensor_id}/setState` - Set switch state ("true"/"false")

**Publishes to:**
- `{mqtt_header}{sensor_id}/state` - Current state ("true"/"false")

#### LedSensor

Extends SwitchSensor with brightness control:

```cpp
LedSensor(
    const char *sensor_id,
    const char *sensor_name,
    const char *sensor_type = "led",
    const char *metadata = "{}"
);
```

**Subscribes to:**
- `{mqtt_header}{sensor_id}/setState` - Set on/off state
- `{mqtt_header}{sensor_id}/aux/setBrightness` - Set brightness (0.0-1.0)
- `{mqtt_header}{sensor_id}/aux/initialBrightness` - Set initial brightness

**Publishes to:**
- `{mqtt_header}{sensor_id}/state` - Current state
- `{mqtt_header}{sensor_id}/aux/brightness` - Current brightness

#### RgbSensor

Extends LedSensor with color control:

```cpp
RgbSensor(
    const char *sensor_id,
    const char *sensor_name
);
```

**Additional Subscriptions:**
- `{mqtt_header}{sensor_id}/aux/setColor` - Set color (hex format)

**Additional Publications:**
- `{mqtt_header}{sensor_id}/aux/color` - Current color

#### Thermostat

Extends AnalogSensor with heating control:

```cpp
Thermostat(
    const char *sensor_id,
    const char *sensor_name,
    const unsigned int report_value_period = 30000,
    const unsigned short decimals = 2,
    const bool enable_filter = true,
    const char *sensor_type = "thermostat",
    const char *metadata = "{}",
    const float filter_rate_change = 0.1
);
```

**Subscribes to:**
- `{mqtt_header}{sensor_id}/setState` - Enable/disable thermostat
- `{mqtt_header}{sensor_id}/aux/setHeating` - Set heating state

**Publishes to:**
- `{mqtt_header}{sensor_id}/value` - Current temperature
- `{mqtt_header}{sensor_id}/state` - Thermostat enabled state
- `{mqtt_header}{sensor_id}/aux/heating` - Current heating state

#### Toggle

For tri-state toggle controls (blinds, curtains, etc.):

```cpp
Toogle(  // Note: typo in original code
    const char *sensor_id,
    const char *sensor_name,
    const char *metadata = "{}"
);
```

**Subscribes to:**
- `{mqtt_header}{sensor_id}/aux/setToogle` - Set action ("up", "down", "toogle")
- `{mqtt_header}{sensor_id}/aux/actionTime` - Set action duration (ms)

## Configuration Mode

When the device has no valid configuration (first boot or after factory reset), it enters **Setup Mode**:

1. **WiFi Access Point** is created:
   - SSID: `iot_XXXX` (where XXXX is derived from MAC address)
   - Password: `12345678`

2. **LED Indicator**: Built-in LED blinks at 1Hz

3. **Web Server Endpoints**:

   | Endpoint | Method | Description |
   |----------|--------|-------------|
   | `/getDeviceInfo` | GET | Get device information JSON |
   | `/deviceSetup` | POST | Configure device with JSON payload |

### Configuration Payload

```json
{
    "deviceId": "unique_device_id",
    "locationId": "location_identifier",
    "token": "mqtt_authentication_token",
    "wifiSSID": "your_wifi_ssid",
    "wifiPassword": "your_wifi_password",
    "subsensorId": "optional_subsensor_id"
}
```

## Runtime Mode

Once configured, the device operates in **Runtime Mode**:

1. **WiFi Connection**: Connects to configured network
2. **MQTT Connection**: Connects to `mqtt.iotcloud.es:1883`
3. **Time Sync**: Synchronizes with NTP server (`pool.ntp.org`)
4. **Sensor Initialization**: All sensors subscribe to their topics
5. **OTA Readiness**: Listens for firmware update commands

### Device Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| `v1/{locationId}/{deviceId}/status` | Publish | Device online status |
| `v1/{locationId}/{deviceId}/ip` | Publish | Device IP address |
| `v1/{locationId}/{deviceId}/version` | Publish | Firmware version |
| `v1/{locationId}/{deviceId}/model` | Publish | Device model |
| `v1/{locationId}/{deviceId}/reset` | Subscribe | Factory reset command |
| `v1/{locationId}/{deviceId}/restart` | Subscribe | Restart command |

## OTA Updates

Over-The-Air updates are performed securely over HTTPS:

1. **OTA Server**: `https://ota.iotcloud.es`
2. **Certificate Pinning**: Uses embedded root CA certificate
3. **Model-Based Updates**: Listens on `v1/ota/update/{device_model}`

### Update Process

1. New version is published to the OTA topic
2. Device compares versions
3. If different, downloads from `https://ota.iotcloud.es/{model}/{version}`
4. LED indicates download progress
5. Device restarts with new firmware

### Disable OTA

```cpp
IotCloud_Constants::ENABLE_OTA = false;
```

## MQTT Topics

### Topic Structure

All topics follow the pattern:
```
v1/{locationId}/{deviceId}/{sensor_id}/{endpoint}
```

### Common Endpoints

| Endpoint | Description |
|----------|-------------|
| `value` | Sensor value (analog sensors) |
| `state` | Boolean state (switches, thermostats) |
| `setState` | Set state command |
| `aux/*` | Auxiliary/configuration endpoints |

### Quality of Service

- All retained messages use QoS 0
- Last Will and Testament: `offline` on status topic

## Factory Reset

Hold the reset button (GPIO 0 / GPIO 9 for ESP32-C3) for **5 seconds** to:

1. Clear all saved configuration from EEPROM
2. Restart device
3. Enter Setup Mode

## Constants Configuration

Located in `iotcloud_constants.h`:

| Constant | Default Value | Description |
|----------|---------------|-------------|
| `MQTT_SERVER` | `mqtt.iotcloud.es` | MQTT broker address |
| `AP_SSID_PREFIX` | `iot` | Access point SSID prefix |
| `AP_PASSWORD` | `12345678` | Access point password |
| `OTA_SERVER` | `ota.iotcloud.es` | OTA server hostname |
| `NTP_SERVER` | `pool.ntp.org` | NTP server |
| `ENABLE_OTA` | `true` | Enable/disable OTA updates |

## Examples

### Custom Analog Sensor

```cpp
#include <iotcloud.h>
#include <sensors/analog.h>
#include <DHT.h>

class TemperatureSensor : public AnalogSensor {
protected:
    DHT dht;
    
    void get_value() override {
        float temp = dht.readTemperature();
        if (!isnan(temp)) {
            set_value(temp);
        }
    }
    
public:
    TemperatureSensor(const char* id, const char* name, uint8_t pin)
        : AnalogSensor(id, name, 60000, 1), dht(pin, DHT22) {
        dht.begin();
    }
};

Device device("1.0.0", "temp_sensor");
TemperatureSensor temp("temp1", "Room Temperature", D4);

void setup() {
    Serial.begin(115200);
    device.add_sensor(&temp);
    iotcloud_setup(&device);
}

void loop() {
    iotcloud_loop();
}
```

### Smart Relay

```cpp
#include <iotcloud.h>
#include <sensors/switch.h>

#define RELAY_PIN 5

class RelaySwitch : public SwitchSensor {
protected:
    uint8_t _pin;
    
    void set_state(bool new_state) override {
        digitalWrite(_pin, new_state ? HIGH : LOW);
        SwitchSensor::set_state(new_state);
    }
    
public:
    RelaySwitch(const char* id, const char* name, uint8_t pin)
        : SwitchSensor(id, name), _pin(pin) {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
    }
};

Device device("1.0.0", "smart_relay");
RelaySwitch relay("relay1", "Main Relay", RELAY_PIN);

void setup() {
    Serial.begin(115200);
    device.add_sensor(&relay);
    iotcloud_setup(&device);
}

void loop() {
    iotcloud_loop();
}
```

## Troubleshooting

### Device Won't Connect to WiFi
- Verify WiFi credentials in configuration
- Check if WiFi network is 2.4GHz (ESP doesn't support 5GHz)
- Ensure signal strength is adequate
- Try factory reset and reconfigure

### MQTT Connection Issues
- Verify token is valid
- Check MQTT server connectivity
- Ensure device ID is unique
- Review debug output (enable `#define DEBUG`)

### OTA Update Fails
- Check device model matches OTA topic
- Verify firmware binary exists on OTA server
- Ensure sufficient flash memory
- Check TLS certificate validity

### Enable Debug Mode

Add before including the library:
```cpp
#define DEBUG
#include <iotcloud.h>
```

This enables Serial output for:
- Configuration data
- MQTT connection status
- OTA update progress
- Sensor value changes

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

This library is licensed under the **GNU Lesser General Public License v2.1**.

See [LICENSE](LICENSE) for more information.

---

## Author

**Cesar Sanz** - [cesarsanz.91@gmail.com](mailto:cesarsanz.91@gmail.com)

GitHub: [https://github.com/csanz91/IotCloud-arduino-lib](https://github.com/csanz91/IotCloud-arduino-lib)

---

*Part of the IotCloud Home Automation Platform*
