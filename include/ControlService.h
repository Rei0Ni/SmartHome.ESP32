#include <ArduinoJson.h>
#include <vector>
#include <map>

#ifndef SerialService_h
#include <SerialService.h>
#endif

#ifndef ControlService_h
#define ControlService_h

// Define pin types
typedef enum {
    PIN_TYPE_LED,
    PIN_TYPE_DHT_SENSOR,
    PIN_TYPE_FAN,
    PIN_TYPE_RELAY,
    PIN_TYPE_OTHER
} DeviceType;

// Structure for the lookup table
typedef struct
{
  const char *name; // Macro name as a string
  int value;        // Macro value
  int mode;      // Pin mode (input/output)
  DeviceType type;      // Pin type (e.g., LED, SENSOR, FAN)
} DeviceEntry;

class SerialService;  // Forward declaration

class ControlService
{
private:
    SerialService *ss;

     // Use a nested map: area-id -> (device_id -> DeviceEntry)
    std::map<std::string, std::map<std::string, DeviceEntry>> areaDevicesMap;

    bool toggle(int pin, int state);
    // Modified declarePin and getPinValue for the map structure
    void declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type);
    int getPinValue(const char *areaId, const char *deviceId); // Get pin by area and deviceId
    
public:
    ControlService(SerialService *ss);
    ~ControlService();
    void handleCommand(JsonDocument doc, JsonDocument &response);
    void configurePins();
};

#endif