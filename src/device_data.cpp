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

device_data get_data()
{
    device_data data;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(INITIAL_ADDR, data);
    EEPROM.end();

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