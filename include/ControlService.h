#ifndef ControlService_h
#define ControlService_h

#include <Arduino.h>
#include <map>
#include <string>
#include <DHT.h>
#include <ArduinoJson.h>

class SerialService; // Forward declaration

// Define device types enum
enum DeviceType
{
    PIN_TYPE_LED,
    PIN_TYPE_FAN,
    PIN_TYPE_DHT11,
    PIN_TYPE_PIR,
    PIN_TYPE_OTHER
};

// Structure to hold device information
struct DeviceEntry {
    const char* deviceId;
    int value;
    int mode;
    DeviceType type;
    int pwmChannel;

    // Default constructor
    DeviceEntry() : deviceId(nullptr), value(0), mode(0), type(PIN_TYPE_OTHER), pwmChannel(-1) {}

    DeviceEntry(const char* deviceId, int value, int mode, DeviceType type, int pwmChannel)
        : deviceId(deviceId), value(value), mode(mode), type(type), pwmChannel(pwmChannel) {}
};


class ControlService
{
private:
    SerialService *ss; // Pointer to SerialService

    // Device Management
    std::map<std::string, std::map<std::string, DeviceEntry>> areaDevicesMap;
    std::map<int, int> digitalPinStates; // Store digital pin states
    std::map<int, int> fanSpeeds;         // Store fan speeds (percentage)
    std::map<int, int> ledBrightness;       // Store LED brightness (5-100)
    std::map<int, int> lastActiveFanSpeeds; // Stores the last non-zero speed percentage
    std::map<int, int> lastActiveLedBrightness; // Stores the last non-zero brightness percentage


    // PWM Configuration - ESP32 LEDC
    const int pwmFrequencyHz = 25000; // PWM frequency (e.g., 25 kHz for fans)
    const int pwmResolutionBits = 8;   // PWM resolution (8 bits = 0-255)
    static const int maxPwmChannels = 16; // ESP32 has 16 LEDC channels
    int nextPwmChannel = 0; // Track next available channel

    // DHT Sensor Management
    std::map<int, DHT *> dhtSensors; // Map of DHT sensor objects, key is pin number

    int getNextAvailablePwmChannel();

public:
    ControlService(SerialService *ss); // Constructor
    ~ControlService();                       // Destructor

    void Initialize();

    void declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type); // Declare a device pin
    int getPinValue(const char *areaId, const char *deviceId);                                        // Get pin value for a device
    DeviceType getDeviceType(const char *areaId, const char *deviceId);                                // Get device type
    void handleCommand(JsonDocument doc, JsonDocument &response);                                    // Handle JSON commands
    String getAllSensorDataJson();

    void declareDevices();

    bool toggle(int pin, int state);                                               // Toggle digital pin state
    bool controlFanSpeed(int pin, int speedPercentage);                              // Control fan speed using PWM
    bool getDHT11Readings(int pin, float &temperature, float &humidity);          // Read DHT11 sensor data
    bool getPIRState(int pin, bool &motionDetected);                                 // Read PIR sensor state
    bool controlLedBrightness(int pin, int brightness);                            // Control LED brightness
};

#endif // ControlService_h