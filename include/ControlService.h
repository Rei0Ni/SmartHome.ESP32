#ifndef ControlService_h
#define ControlService_h

#include <Arduino.h>
#include <map>
#include <string>
#include <DHT.h> // Include DHT library
#include <ArduinoJson.h>

class SerialService; // Forward declaration

// Define device types enum
enum DeviceType
{
    PIN_TYPE_LED,
    PIN_TYPE_FAN,
    PIN_TYPE_DHT11, // DHT11 Temperature/Humidity Sensor
    PIN_TYPE_PIR,   // PIR Motion Sensor
    PIN_TYPE_OTHER
};

// Structure to hold device information
struct DeviceEntry
{
    const char *deviceId;
    int value;
    int mode;
    DeviceType type;
};

class ControlService
{
private:
    SerialService *ss; // Pointer to SerialService

    // Device Management
    std::map<std::string, std::map<std::string, DeviceEntry>> areaDevicesMap;
    std::map<int, int> digitalPinStates; // Store digital pin states
    std::map<int, int> fanSpeeds;        // Store fan speeds (percentage)

    // PWM Configuration - ESP32 LEDC
    const int pwmChannel = 0;         // LEDC channel (0-15)
    const int pwmFrequencyHz = 25000; // PWM frequency (e.g., 25 kHz for fans)
    const int pwmResolutionBits = 8;  // PWM resolution (8 bits = 0-255)

    // DHT Sensor Management
    std::map<int, DHT *> dhtSensors; // Map of DHT sensor objects, key is pin number

    void setupPWM(); // Configure PWM
    void configurePins(); // Configure pin modes from device map

public:
    ControlService(SerialService *ss); // Constructor
    ~ControlService();                 // Destructor

    void declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type); // Declare a device pin
    int getPinValue(const char *areaId, const char *deviceId);                                       // Get pin value for a device
    DeviceType getDeviceType(const char *areaId, const char *deviceId);                              // Get device type
    void handleCommand(JsonDocument doc, JsonDocument &response);                                    // Handle JSON commands
    String getAllSensorDataJson();

    bool toggle(int pin, int state);                                     // Toggle digital pin state
    bool controlFanSpeed(int pin, int speedPercentage);                  // Control fan speed using PWM
    bool getDHT11Readings(int pin, float &temperature, float &humidity); // Read DHT11 sensor data
    bool getPIRState(int pin, bool &motionDetected);                     // Read PIR sensor state
};

#endif // ControlService_h