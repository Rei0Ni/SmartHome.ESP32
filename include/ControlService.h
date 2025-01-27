#include <ArduinoJson.h>
#include <vector>

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
    PIN_TYPE_OTHER
} PinType;

// Structure for the lookup table
typedef struct
{
  const char *name; // Macro name as a string
  int value;        // Macro value
  int mode;      // Pin mode (input/output)
  PinType type;      // Pin type (e.g., LED, SENSOR, FAN)
} PinEntry;

class SerialService;  // Forward declaration

class ControlService
{
private:
    SerialService *ss;

    std::vector<PinEntry> pinTable;

    bool toggle(int pin, int state);
    void declarePin(const char *name, int value, int mode);
    int getPinValue(const char *pinName);
    
public:
    ControlService(SerialService *ss);
    ~ControlService();
    void handleCommand(JsonDocument doc, JsonDocument &response);
    void configurePins();
};

#endif