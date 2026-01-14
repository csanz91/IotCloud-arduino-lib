#ifndef _IOTCLOUD_DEVICE_DATA
#define _IOTCLOUD_DEVICE_DATA

#define TOKEN_MAX_SIZE 396
#define DEVICEID_MAX_SIZE 33
#define WIFI_SSID_MAX_SIZE 33
#define WIFI_PASSWORD_MAX_SIZE 64
#define LOCATIONID_MAX_SIZE 26
#define SUBSENSORID_MAX_SIZE 64
#define SUBDEVICEID_MAX_SIZE 33

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

extern void clear_saved_data();
extern void save_data(const device_data *);
extern device_data get_data();
extern bool validate_device_data(const device_data *data);
extern void pretty_print_data(const device_data *data);

#endif // _IOTCLOUD_DEVICE_DATA