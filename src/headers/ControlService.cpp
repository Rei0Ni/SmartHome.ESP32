#include "ControlService.h"
#include <cstring>

#ifndef _PREFERENCES_H_
#include <Preferences.h>
#endif

#include <Arduino.h> // Make sure to include Arduino.h for ESP32 functions

using namespace std;

/// @brief Sets up the PWM configuration using ESP32 LEDC (LED Control)
void ControlService::setupPWM() {
    // Configure LEDC PWM parameters
    ledcSetup(pwmChannel, pwmFrequencyHz, pwmResolutionBits); // Channel, Frequency, Resolution
}

/// @brief Toggles the state of a device pin (basic ON/OFF control - digitalWrite)
bool ControlService::toggle(int pin, int state) {
    try {
        digitalWrite(pin, state);
        digitalPinStates[pin] = state; // Store the digital pin state
        return true;
    } catch (const exception& e) {
        // ss->printToAll("Error while toggling device state: %S", e.what());
        return false;
    }
}

/// @brief Controls fan speed using PWM with ESP32 LEDC (analogWrite replacement)
bool ControlService::controlFanSpeed(int pin, int speedPercentage) {
    try {
        if (speedPercentage >= 0 && speedPercentage <= 100) {
            // Map the 0-100 speed percentage to the 0-255 PWM range
            int pwmValue = ::map(speedPercentage, 0, 100, 0, 255);

            // Use ledcWrite with the mapped PWM value
            ledcWrite(pwmChannel, pwmValue);
            fanSpeeds[pin] = speedPercentage; // Store the fan speed
            return true;
        } else {
            // ss->printToAll("Error: Invalid fan speed percentage. Must be between 0 and 100.");
            return false;
        }
    } catch (const exception& e) {
        // ss->printToAll("Error while controlling fan speed: %S", e.what());
        return false;
    }
}

/// @brief Declares a pin for a device (same as before)
void ControlService::declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type) {
    DeviceEntry entry = {deviceId, value, mode, type};
    areaDevicesMap[areaId][deviceId] = entry;
}

/// @brief Gets pin value for a device (same as before)
int ControlService::getPinValue(const char *areaId, const char *deviceId) {
    if (areaDevicesMap.count(areaId)) {
        if (areaDevicesMap[areaId].count(deviceId)) {
            return areaDevicesMap[areaId][deviceId].value;
        } else {
            // ss->printToAll("Device ID '%s' not found in area '%s'", deviceId, areaId);
        }
    } else {
        // ss->printToAll("Area '%s' not found", areaId);
    }
    return -1;
}

/// @brief Gets device type (new function)
DeviceType ControlService::getDeviceType(const char *areaId, const char *deviceId) {
    if (areaDevicesMap.count(areaId)) {
        if (areaDevicesMap[areaId].count(deviceId)) {
            return areaDevicesMap[areaId][deviceId].type;
        }
    }
    return PIN_TYPE_OTHER; // Default to generic if not found
}


/// @brief Configures pin modes (same as before)
void ControlService::configurePins() {
    for (auto const& areaPair : areaDevicesMap) {
        for (auto const& devicePair : areaPair.second) {
            pinMode(devicePair.second.value, devicePair.second.mode);
        }
    }
}

/// @brief Constructor (Modified to call setupPWM)
ControlService::ControlService(SerialService *ss) : ss(ss) { // Use initializer list
    // Initialize SerialService pointer
    this->ss = ss; // No longer needed with initializer list

    // Initialize PWM setup using LEDC
    setupPWM();

    // Declare devices - Using pin 23 for PWM fan control (make sure pin 23 is still connected to L298N ENA)
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "1bd59658-ba07-4520-b2c3-6cc7df314d4c", 18, OUTPUT, PIN_TYPE_LED);
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "ee72372d-253b-4775-85e4-9ff851a343a0", 19, OUTPUT, PIN_TYPE_LED);
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "fan_1_id", 5, OUTPUT, PIN_TYPE_FAN); // Fan on pin 5 (PWM)

    configurePins();
    // Attach PWM channel to the fan pin *after* configuring pin modes
    ledcAttachPin(getPinValue("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "fan_1_id"), pwmChannel); // Pin, Channel
}

/// @brief Destructor (same as before)
ControlService::~ControlService() {}

/// @brief Handles JSON commands (Modified - no changes in command handling logic itself)
void ControlService::handleCommand(JsonDocument doc, JsonDocument &response) {
    // ... (handleCommand function - same logic as in the previous complete code) ...
    // No changes needed in the command handling logic, just make sure it calls controlFanSpeed
    // for "setspeed" function.
    const char *areaId = doc["areaId"];
    if (areaId == nullptr) {
        response["status"] = "error";
        response["message"] = "Missing 'areaId' in command";
        return;
    }

    if (!doc.containsKey("devices") || !doc["devices"].is<JsonArray>()) {
        response["status"] = "error";
        response["message"] = "Missing or invalid 'devices' array in command";
        return;
    }

    if (!areaDevicesMap.count(areaId)) {
        response["status"] = "error";
        response["message"] = String("Area '") + areaId + "' not found";
        return;
    }

    JsonArray devicesResponse = response.createNestedArray("devices"); // Create an array for device responses

    for (JsonObject device : doc["devices"].as<JsonArray>()) {
        const char *device_id = device["deviceId"];
        const char *function_name = device["function"];
        JsonObject deviceResponse = devicesResponse.createNestedObject(); // Create response object for each device
        deviceResponse["deviceId"] = device_id; // Add deviceId to device response

        if (device_id == nullptr || function_name == nullptr) {
            deviceResponse["status"] = "error";
            deviceResponse["message"] = "Missing 'deviceId' or 'function' in device command";
            continue;
        }

        if (strcmp(function_name, "toggle") == 0) {
            JsonObject parameters = device["parameters"];
            if (!parameters.containsKey("state")) {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = "Missing 'state' parameter for 'toggle' function";
                continue;
            }

            bool state = parameters["state"];
            int level = (state == true) ? HIGH : LOW;

            int pin = getPinValue(areaId, device_id);
            if (pin == -1) {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Invalid device ID '") + device_id + "' or area '" + areaId + "'";
                continue;
            }

            if (this->toggle(pin, level)) {
                deviceResponse["status"] = "success";
                deviceResponse["message"] = String("Toggled device '") + device_id + "' to state " + (state ? "on" : "off");
                deviceResponse["power_state"] = (state ? "on" : "off"); // Add power_state to response
            } else {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Toggle failed for device '") + device_id + "'";
            }
        } else if (strcmp(function_name, "setspeed") == 0) {
            JsonObject parameters = device["parameters"];
            if (!parameters.containsKey("speed")) {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = "Missing 'speed' parameter for 'setspeed' function";
                continue;
            }

            int speedPercentage = parameters["speed"]; // **Expect speed as an integer percentage (0-100) now**

            int pin = getPinValue(areaId, device_id);
            if (pin == -1) {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Invalid device ID '") + device_id + "' or area '" + areaId + "' for fan speed control";
                continue;
            }

            if (this->controlFanSpeed(pin, speedPercentage)) { // **Call controlFanSpeed with speedPercentage**
                deviceResponse["status"] = "success";
                deviceResponse["message"] = String("Set fan '") + device_id + "' speed to " + speedPercentage + "%"; // Update message to show percentage
                deviceResponse["fan_speed"] = speedPercentage; // Add fan_speed to response
                deviceResponse["power_state"] = (speedPercentage > 0) ? "on" : "off"; // Add power_state to response
            } else {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Failed to set speed for fan '") + device_id + "'";
            }
        } else {
            // ss->printToAll("Unknown function '%s' for device '%s'", function_name, device_id);
            deviceResponse["status"] = "error";
            deviceResponse["message"] = "Unknown function!";
        }
    }
}