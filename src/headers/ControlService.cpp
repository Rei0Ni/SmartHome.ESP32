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
    DeviceEntry entry = {deviceId, value, mode, type}; // DeviceId used as name in DeviceEntry
    areaDevicesMap[areaId][deviceId] = entry; // Insert into nested map
}

/// @brief Gets the pin value for a specific device in a specific area
/// @param areaId The ID of the area
/// @param deviceId The ID of the device
/// @return The pin value, or -1 if the area or device is not found
int ControlService::getPinValue(const char *areaId, const char *deviceId)
{
    if (areaDevicesMap.count(areaId)) // Check if area exists
    {
        if (areaDevicesMap[areaId].count(deviceId)) // Check if device exists in area
        {
            return areaDevicesMap[areaId][deviceId].value;
        }
        else
        {
            ss->printToAll("Device ID '%s' not found in area '%s'", deviceId, areaId);
        }
    }
    else
    {
        ss->printToAll("Area '%s' not found", areaId);
    }
    return -1; // Return -1 if area or device not found
}

/// @brief Configures the pins for all declared devices
void ControlService::configurePins()
{
    for (auto const& areaPair : areaDevicesMap) // Iterate through areas
    {
        for (auto const& devicePair : areaPair.second) // Iterate through devices in each area
        {
            pinMode(devicePair.second.value, devicePair.second.mode);
        }
    }
}

/// @brief Constructor for ControlService
/// @param ss Pointer to the SerialService instance
ControlService::ControlService(SerialService *ss)
{
    this->ss = ss;
    // Declare devices with their pins, modes, and types
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "1bd59658-ba07-4520-b2c3-6cc7df314d4c", 18, OUTPUT, PIN_TYPE_LED);
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "ee72372d-253b-4775-85e4-9ff851a343a0", 19, OUTPUT, PIN_TYPE_LED);
    configurePins();
}

/// @brief Destructor for ControlService
ControlService::~ControlService()
{
}

/// @brief Handles a command received in JSON format using the new CommandService structure.
/// @param doc The JSON document containing the command.
/// @param response The JSON document to store the response.
void ControlService::handleCommand(JsonDocument doc, JsonDocument &response)
{
    // Extract the area ID from the command
    const char *areaId = doc["areaId"];
    if (areaId == nullptr)
    {
        response["status"] = "error";
        response["message"] = "Missing 'areaId' in command";
        return;
    }

    // Ensure the devices array is present
    if (!doc.containsKey("devices") || !doc["devices"].is<JsonArray>())
    {
        response["status"] = "error";
        response["message"] = "Missing or invalid 'devices' array in command";
        return;
    }

    // Check if the specified area exists in our map
    if (!areaDevicesMap.count(areaId))
    {
        response["status"] = "error";
        response["message"] = String("Area '") + areaId + "' not found";
        return;
    }

    // Process each device command in the devices array
    for (JsonObject device : doc["devices"].as<JsonArray>())
    {
        const char *device_id = device["deviceId"];
        const char *function_name = device["function"];

        if (device_id == nullptr || function_name == nullptr)
        {
            response["status"] = "error";
            response["message"] = "Missing 'deviceId' or 'function' in one of the device commands";
            continue;
        }

        // Execute command based on function type
        if (strcmp(function_name, "toggle") == 0)
        {
            JsonObject parameters = device["parameters"];
            if (!parameters.containsKey("state"))
            {
                response["status"] = "error";
                response["message"] = "Missing 'state' parameter";
                continue;
            }
            
            // Expect state as an integer (e.g., 1 for HIGH, 0 for LOW)
            int state = parameters["state"];
            int level = (state == true) ? HIGH : LOW;

            int pin = getPinValue(areaId, device_id); // Retrieve the pin for the device
            if (pin == -1)
            {
                response["status"] = "error";
                response["message"] = String("Invalid device ID '") + device_id + "' or area '" + areaId + "'";
                continue;
            }

            if (this->toggle(pin, level))
            {
                response["status"] = "Success";
                response["message"] = String("Toggled device '") + device_id + "' in area '" + areaId + "' to state " + state;
            }
            else
            {
                response["status"] = "error";
                response["message"] = String("Toggle failed for device '") + device_id + "' in area '" + areaId + "'";
            }
        }
        else
        {
            ss->printToAll("Unknown function '%s' for device '%s'", function_name, device_id);
            response["status"] = "error";
            response["message"] = "Unknown function!";
        }
    }
}
