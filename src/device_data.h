#ifndef _IOTCLOUD_DEVICE_DATA
#define _IOTCLOUD_DEVICE_DATA

#include <stdint.h>

// Current struct sizes
#define TOKEN_MAX_SIZE 396
#define DEVICEID_MAX_SIZE 33
#define WIFI_SSID_MAX_SIZE 33
#define WIFI_PASSWORD_MAX_SIZE 64
#define LOCATIONID_MAX_SIZE 26
#define SUBSENSORID_MAX_SIZE 64
#define SUBDEVICEID_MAX_SIZE 33

// Version of the device_data layout stored in EEPROM. When the struct layout
// changes, bump it and convert the old version in get_data().
#define DEVICE_DATA_VERSION 1

// Legacy struct sizes (for migration from old firmware)
#define LEGACY_TOKEN_MAX_SIZE 290

struct device_data
{
    char token[TOKEN_MAX_SIZE] = "";
    char deviceId[DEVICEID_MAX_SIZE] = "";
    char wifiSSID[WIFI_SSID_MAX_SIZE] = "";
    char wifiPassword[WIFI_PASSWORD_MAX_SIZE] = "";
    char locationId[LOCATIONID_MAX_SIZE] = "";
    char subsensorId[SUBSENSORID_MAX_SIZE] = "";  // Optional: for subsensor devices to publish to main thermostat's topic
    char subdeviceId[SUBDEVICEID_MAX_SIZE] = "";
};

// Previous Wi-Fi credentials, kept while a Wi-Fi change received over MQTT
// has not been confirmed yet. Stored in EEPROM after device_data.
struct network_fallback
{
    uint32_t magic = 0;
    char wifiSSID[WIFI_SSID_MAX_SIZE] = "";
    char wifiPassword[WIFI_PASSWORD_MAX_SIZE] = "";
};

extern void clear_saved_data();
extern void save_network_fallback(network_fallback *fallback);
extern bool get_network_fallback(network_fallback *fallback); // true if a change is pending
extern void clear_network_fallback();
extern void save_data(const device_data *);
extern device_data get_data();
extern bool validate_device_data(const device_data *data);
extern void pretty_print_data(const device_data *data);

#endif // _IOTCLOUD_DEVICE_DATA