#include <EEPROM.h>
#include <Arduino.h>
#include "IotCloud_config.h"
#include "device_data.h"

#define EEPROM_SIZE 1024
#define INITIAL_ADDR 0
#define NETWORK_FALLBACK_ADDR 700
#define NETWORK_FALLBACK_MAGIC 0x4E455431UL // "NET1"

#define STORAGE_VERSION_ADDR 900
#define STORAGE_VERSION_MAGIC 0x494F5456UL // "IOTV"

// Records which device_data layout is stored, so future layout changes don't
// have to guess the format. Firmware before this marker didn't write it.
struct storage_version
{
    uint32_t magic = 0;
    uint16_t version = 0;
};

static_assert(INITIAL_ADDR + sizeof(device_data) <= NETWORK_FALLBACK_ADDR,
              "network_fallback overlaps device_data in EEPROM");
static_assert(NETWORK_FALLBACK_ADDR + sizeof(network_fallback) <= STORAGE_VERSION_ADDR,
              "storage_version overlaps network_fallback in EEPROM");
static_assert(STORAGE_VERSION_ADDR + sizeof(storage_version) <= EEPROM_SIZE,
              "storage_version does not fit in EEPROM");

// Call these between EEPROM.begin() and EEPROM.end()
static void write_storage_version()
{
    storage_version marker;
    marker.magic = STORAGE_VERSION_MAGIC;
    marker.version = DEVICE_DATA_VERSION;
    EEPROM.put(STORAGE_VERSION_ADDR, marker);
}

// 0 means no marker: the data was written by firmware older than the marker
static uint16_t read_storage_version()
{
    storage_version marker;
    EEPROM.get(STORAGE_VERSION_ADDR, marker);
    return marker.magic == STORAGE_VERSION_MAGIC ? marker.version : 0;
}

void clear_saved_data()
{
    device_data data;
    network_fallback fallback;
    EEPROM.begin(EEPROM_SIZE);
    storage_version no_marker;
    EEPROM.put(INITIAL_ADDR, data);
    EEPROM.put(NETWORK_FALLBACK_ADDR, fallback);
    EEPROM.put(STORAGE_VERSION_ADDR, no_marker);
    EEPROM.end();
}

void save_network_fallback(network_fallback *fallback)
{
    fallback->magic = NETWORK_FALLBACK_MAGIC;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(NETWORK_FALLBACK_ADDR, *fallback);
    EEPROM.end();
}

bool get_network_fallback(network_fallback *fallback)
{
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(NETWORK_FALLBACK_ADDR, *fallback);
    EEPROM.end();

    fallback->wifiSSID[WIFI_SSID_MAX_SIZE - 1] = '\0';
    fallback->wifiPassword[WIFI_PASSWORD_MAX_SIZE - 1] = '\0';
    return fallback->magic == NETWORK_FALLBACK_MAGIC && fallback->wifiSSID[0] != '\0';
}

void clear_network_fallback()
{
    network_fallback fallback;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(NETWORK_FALLBACK_ADDR, fallback);
    EEPROM.end();
}

void save_data(const device_data *data)
{
    EEPROM.begin(EEPROM_SIZE);
#ifdef DEBUG
    pretty_print_data(data);
#endif
    EEPROM.put(INITIAL_ADDR, *data);
    write_storage_version();
    EEPROM.end();
}

// Legacy struct for reading old EEPROM format (without subsensor fields, smaller token)
struct device_data_legacy
{
    char token[LEGACY_TOKEN_MAX_SIZE];
    char deviceId[DEVICEID_MAX_SIZE];
    char wifiSSID[WIFI_SSID_MAX_SIZE];
    char wifiPassword[WIFI_PASSWORD_MAX_SIZE];
    char locationId[LOCATIONID_MAX_SIZE];
};

static bool validate_data_fields(const char* deviceId, const char* locationId, 
                                  const char* token, const char* wifiSSID, 
                                  const char* wifiPassword)
{
    return deviceId[0] != '\0' && isAlphaNumeric(deviceId[0]) &&
           locationId[0] != '\0' && isAlphaNumeric(locationId[0]) &&
           token[0] != '\0' && isAlphaNumeric(token[0]) &&
           wifiPassword[0] != '\0' &&
           wifiSSID[0] != '\0';
}

// Helper to sanitize optional string fields that may contain garbage from old EEPROM data
static void sanitize_optional_field(char* field, size_t max_size)
{
    if (field[0] != '\0' && !isAlphaNumeric(field[0])) {
        field[0] = '\0';
    }
    field[max_size - 1] = '\0';
}

device_data get_data()
{
    device_data data;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(INITIAL_ADDR, data);
    
    // Check if new format is valid
    bool valid = validate_data_fields(data.deviceId, data.locationId, 
                                       data.token, data.wifiSSID, data.wifiPassword);
    uint16_t version = read_storage_version();
    if (version != 0 && version != DEVICE_DATA_VERSION)
    {
        // Written by newer firmware (e.g. after a downgrade). Read it as the
        // current layout, as before the marker existed, and leave the marker.
        IOTCLOUD_LOG_PRINTLN(F("Unknown EEPROM data version; reading as current format"));
    }

    // Without a marker the format is unknown: it may be the legacy layout
    if (!valid && version == 0) {
        // Try reading with legacy format (smaller token, no subsensor fields)
        device_data_legacy legacy;
        EEPROM.get(INITIAL_ADDR, legacy);
        
        if (validate_data_fields(legacy.deviceId, legacy.locationId,
                                  legacy.token, legacy.wifiSSID, legacy.wifiPassword)) {
            // Migration: copy legacy data to new format
            IOTCLOUD_LOG_PRINTLN(F("Migrating legacy EEPROM data to new format..."));
            memset(&data, 0, sizeof(data));
            
            strncpy(data.token, legacy.token, LEGACY_TOKEN_MAX_SIZE);
            data.token[LEGACY_TOKEN_MAX_SIZE] = '\0';  // Ensure null-termination
            
            strncpy(data.deviceId, legacy.deviceId, DEVICEID_MAX_SIZE - 1);
            data.deviceId[DEVICEID_MAX_SIZE - 1] = '\0';
            
            strncpy(data.wifiSSID, legacy.wifiSSID, WIFI_SSID_MAX_SIZE - 1);
            data.wifiSSID[WIFI_SSID_MAX_SIZE - 1] = '\0';
            
            strncpy(data.wifiPassword, legacy.wifiPassword, WIFI_PASSWORD_MAX_SIZE - 1);
            data.wifiPassword[WIFI_PASSWORD_MAX_SIZE - 1] = '\0';
            
            strncpy(data.locationId, legacy.locationId, LOCATIONID_MAX_SIZE - 1);
            data.locationId[LOCATIONID_MAX_SIZE - 1] = '\0';
            
            // Save in new format
            EEPROM.put(INITIAL_ADDR, data);
            write_storage_version();
            EEPROM.commit();
            IOTCLOUD_LOG_PRINTLN(F("Migration complete."));
        }
    }
    else if (valid && version == 0) {
        // First boot on firmware with the marker: record the current layout
        write_storage_version();
        EEPROM.commit();
    }
    
    EEPROM.end();

    // Sanitize optional fields
    sanitize_optional_field(data.subsensorId, SUBSENSORID_MAX_SIZE);
    sanitize_optional_field(data.subdeviceId, SUBDEVICEID_MAX_SIZE);

    return data;
}

bool validate_device_data(const device_data *data)
{

    return data->deviceId[0] != '\0' && isAlphaNumeric(data->deviceId[0]) &&
           data->locationId[0] != '\0' && isAlphaNumeric(data->locationId[0]) &&
           data->token[0] != '\0' && isAlphaNumeric(data->token[0]) &&
           data->wifiPassword[0] != '\0' &&
           data->wifiSSID[0] != '\0';
}

void pretty_print_data(const device_data *data)
{

    IOTCLOUD_LOG_PRINTLN();
    IOTCLOUD_LOG_PRINT(F("DeviceID: "));
    IOTCLOUD_LOG_PRINTLN(data->deviceId);

    IOTCLOUD_LOG_PRINT(F("LocationID: "));
    IOTCLOUD_LOG_PRINTLN(data->locationId);

    IOTCLOUD_LOG_PRINT(F("Token: "));
    IOTCLOUD_LOG_PRINTLN(data->token);

    IOTCLOUD_LOG_PRINT(F("wifi SSID: "));
    IOTCLOUD_LOG_PRINTLN(data->wifiSSID);

    IOTCLOUD_LOG_PRINT(F("wifi password: "));
    IOTCLOUD_LOG_PRINTLN(data->wifiPassword);

    IOTCLOUD_LOG_PRINT(F("SubsensorID: "));
    IOTCLOUD_LOG_PRINTLN(data->subsensorId);

    IOTCLOUD_LOG_PRINT(F("SubdeviceID: "));
    IOTCLOUD_LOG_PRINTLN(data->subdeviceId);
}
