#ifndef _IOTCLOUD_CONSTANTS
#define _IOTCLOUD_CONSTANTS

namespace IotCloud_Constants
{
    extern const char MQTT_SERVER[];
    extern const char AP_SSID_PREFIX[];
    extern const char AP_PASSWORD[];
    extern const char OTA_SERVER[];
    extern const char NTP_SERVER[];
    extern char DEVICE_INTERNAL_ID[];
    extern char AP_SSID[];
    extern bool ENABLE_OTA;

    void setup_constants();
} // namespace IotCloud_Constants

#endif // _IOTCLOUD_CONSTANTS