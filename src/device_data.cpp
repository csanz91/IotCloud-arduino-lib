#include <EEPROM.h>
#include <Arduino.h>
#include "device_data.h"

#define EEPROM_SIZE 1024
#define INITIAL_ADDR 0

void clear_saved_data()
{
    device_data data;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(INITIAL_ADDR, data);
    EEPROM.end();
}

void save_data(const device_data *data)
{
    EEPROM.begin(EEPROM_SIZE);
#ifdef DEBUG
    pretty_print_data(data);
#endif
    EEPROM.put(INITIAL_ADDR, *data);
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
    
    if (!valid) {
        // Try reading with legacy format (smaller token, no subsensor fields)
        device_data_legacy legacy;
        EEPROM.get(INITIAL_ADDR, legacy);
        
        if (validate_data_fields(legacy.deviceId, legacy.locationId,
                                  legacy.token, legacy.wifiSSID, legacy.wifiPassword)) {
            // Migration: copy legacy data to new format
            Serial.println(F("Migrating legacy EEPROM data to new format..."));
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
            EEPROM.commit();
            Serial.println(F("Migration complete."));
        }
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

    Serial.println();
    Serial.print(F("DeviceID: "));
    Serial.println(data->deviceId);

    Serial.print(F("LocationID: "));
    Serial.println(data->locationId);

    Serial.print(F("Token: "));
    Serial.println(data->token);

    Serial.print(F("wifi SSID: "));
    Serial.println(data->wifiSSID);

    Serial.print(F("wifi password: "));
    Serial.println(data->wifiPassword);

    Serial.print(F("SubsensorID: "));
    Serial.println(data->subsensorId);

    Serial.print(F("SubdeviceID: "));
    Serial.println(data->subdeviceId);
}