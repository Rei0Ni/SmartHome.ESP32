#include <ControlService.h>

#ifndef _PREFERENCES_H_
#include <Preferences.h>
#endif

using namespace std;

/// @brief Toggles the state of a device pin
/// @param pin The pin number to toggle
/// @param state The state to set the pin to (HIGH or LOW)
/// @return true if the operation was successful, false otherwise
bool ControlService::toggle(int pin, int state)
{
    try
    {
        digitalWrite(pin, state);
        return true;
    }
    catch(const exception& e)
    {
        ss->printToAll("Error while toggling device state: %S", e.what());
        return false;
    }
    
}

/// @brief Declares a pin for a device in a specific area
/// @param areaId The ID of the area
/// @param deviceId The ID of the device
/// @param value The pin number
/// @param mode The mode of the pin (INPUT, OUTPUT, etc.)
/// @param type The type of the device
void ControlService::declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type)
{
    DeviceEntry entry = {deviceId, value, mode, type}; // DeviceId as name in PinEntry
    areaDevicesMap[areaId][deviceId] = entry; // Insert into nested map
}

/// @brief Gets the pin value for a specific device in a specific area
/// @param areaId The ID of the area
/// @param deviceId The ID of the device
/// @return The pin value, or -1 if the area or device is not found
int ControlService::getPinValue(const char *areaId, const char *deviceId)
{
    if (areaDevicesMap.count(areaId)) { // Check if area exists
        if (areaDevicesMap[areaId].count(deviceId)) { // Check if deviceId exists in area
            return areaDevicesMap[areaId][deviceId].value;
        } else {
            ss->printToAll("Device ID '%s' not found in area '%s'", deviceId, areaId);
        }
    } else {
        ss->printToAll("Area '%s' not found", areaId);
    }
    return -1; // Return -1 if area or deviceId not found
}

/// @brief Configures the pins for all declared devices
void ControlService::configurePins()
{
    for (auto const& areaPair : areaDevicesMap) { // Iterate through areas
        for (auto const& devicePair : areaPair.second) { // Iterate through devices in each area
            pinMode(devicePair.second.value, devicePair.second.mode);
        }
    }
}

/// @brief Constructor for ControlService
/// @param ss Pointer to the SerialService instance
ControlService::ControlService(SerialService *ss)
{
    this->ss = ss;
    declarePin("Living Room", "LED_1", 18, OUTPUT, PIN_TYPE_LED);
    declarePin("Living Room", "LED_2", 19, OUTPUT, PIN_TYPE_LED);
    configurePins();
}

/// @brief Destructor for ControlService
ControlService::~ControlService()
{
}

/// @brief Handles a command received in JSON format
/// @param doc The JSON document containing the command
/// @param response The JSON document to store the response
void ControlService::handleCommand(JsonDocument doc, JsonDocument &response) {
    // Extract the area
    const char *areaId = doc["area"];

    if (areaId == nullptr) {
        response["status"] = "error";
        response["message"] = "Missing 'area' in command";
        return;
    }

    // Loop through devices in the specified area
    if (areaDevicesMap.count(areaId)) { // Check if the area exists in our map
        for (JsonObject device : doc["devices"].as<JsonArray>()) {
            const char *device_id = device["device_id"];

            // Loop through functions for each device
            for (JsonObject function : device["functions"].as<JsonArray>()) {
                const char *function_name = function["function_name"];

                // Extract parameters
                JsonObject parameters = function["parameters"];

                // Execute the command
                if (strcmp(function_name, "toggle") == 0) {
                    if (parameters["state"].isNull()) {
                        response["status"] = "error";
                        response["message"] = "Missing 'state' parameter";
                        continue;
                    }
                    const char *state = parameters["state"];

                    if (state) {
                        int pin = getPinValue(areaId, device_id); // Use area and device_id to get pin
                        if (pin == -1) {
                            response["status"] = "error";
                            response["message"] = String("Invalid device ID '") + device_id + "' or area '" + areaId + "'";
                            continue; // Skip to next device
                        }
                        int level = !strcmp(state, "on") ? HIGH : LOW;
                        if (this->toggle(pin, level)) {
                            response["status"] = "Success";
                            response["message"] = String("Toggled ") + device_id + " in " + areaId + " to " + state;
                        } else {
                            response["status"] = "error";
                            response["message"] = "Toggle failed for " + String(device_id) + " in " + areaId;
                        }
                    }
                } else {
                    Serial.println("  Unknown function!");
                    response["status"] = "error";
                    response["message"] = "Unknown function!";
                }
            }
        }
    } else {
        response["status"] = "error";
        response["message"] = String("Area '") + areaId + "' not found";
    }
}