// ControlService.h - Header file for the ControlService class
#ifndef ControlService_h
#define ControlService_h

#include <ArduinoJson.h>
#include <vector>
#include <string>
#include <map>
#include <DHT.h>

#ifndef SerialService_h
#include <SerialService.h> // Include SerialService header
#endif

// Define DeviceType enum to categorize device types
typedef enum {
    PIN_TYPE_LED,       // LED device
    PIN_TYPE_DHT_SENSOR, // DHT temperature/humidity sensor (example - not directly controlled by toggle/setspeed)
    PIN_TYPE_FAN,       // Fan device (PWM speed control)
    PIN_TYPE_RELAY,     // Relay device (ON/OFF control)
    PIN_TYPE_OTHER      // Other device type
} DeviceType;

// Structure to represent a device entry in the areaDevicesMap
typedef struct
{
    const char *deviceId; // Unique identifier for the device
    int value;           // Pin number connected to the device (or Enable pin for PWM)
    int mode;            // Pin mode (INPUT, OUTPUT, etc.)
    DeviceType type;     // Type of the device (from DeviceType enum)
} DeviceEntry;

class SerialService; // Forward declaration of SerialService class

class ControlService
{
private:
    SerialService *ss;
    std::map<std::string, std::map<std::string, DeviceEntry>> areaDevicesMap;
    std::map<int, int> digitalPinStates;
    std::map<int, int> fanSpeeds;

    std::map<int, DHT*> dhtSensors;

    // PWM configuration parameters (moved to private members for easy adjustment)
    const int pwmChannel = 0;        // LEDC PWM channel (0-15)
    const int pwmResolutionBits = 8; // PWM resolution (bits, e.g., 8 for 0-255)
    int pwmFrequencyHz = 30000;     // PWM frequency in Hertz (e.g., 20kHz) - **Adjust this value**

public:
    ControlService(SerialService *ss);
    ~ControlService();
    void handleCommand(JsonDocument doc, JsonDocument &response);
    void configurePins();
    DeviceType getDeviceType(const char *areaId, const char *deviceId); // In public - Correct!

    // --- Move these function declarations to the public section ---
    bool toggle(int pin, int state);
    bool controlFanSpeed(int pin, int speed);
    bool getDHT11Readings(int pin, float &temperature, float &humidity);
    void declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type);
    int getPinValue(const char *areaId, const char *deviceId);
    void setupPWM(); // New function to setup LEDC PWM
};

#endif // ControlService_h