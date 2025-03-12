#include "ControlService.h"
#include <cstring>

#ifndef _PREFERENCES_H_
#include <Preferences.h>
#endif

#include <Arduino.h> // Make sure to include Arduino.h for ESP32 functions

// Include DHT sensor library - Make sure you have installed the DHT sensor library in Arduino IDE Library Manager
#include <DHT.h>

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

/// @brief Declares a pin for a device
void ControlService::declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type) {
    DeviceEntry entry = {deviceId, value, mode, type};
    areaDevicesMap[areaId][deviceId] = entry;

    if (type == PIN_TYPE_DHT11) {
        dhtSensors[value] = new DHT(value, DHT11); // Initialize DHT sensor for DHT11 type
        dhtSensors[value]->begin(); // Start DHT sensor
    } else if (type == PIN_TYPE_PIR) {
        // For a PIR sensor, set up the pin mode (e.g., INPUT)
        // Any additional PIR-specific initialization can be added here if needed.
    } else if ( type == PIN_TYPE_FAN) {
        
    }
}

/// @brief Gets pin value for a device
int ControlService::getPinValue(const char *areaId, const char *deviceId) {
    if (areaDevicesMap.count(areaId)) {
        if (areaDevicesMap[areaId].count(deviceId)) {
            return areaDevicesMap[areaId][deviceId].value;
        } 
    }
    return -1;
}

/// @brief Gets device type
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

/// @brief Reads temperature and humidity from DHT11 sensor
bool ControlService::getDHT11Readings(int pin, float &temperature, float &humidity) {
    if (dhtSensors.count(pin)) {
        float h = dhtSensors[pin]->readHumidity();
        float t = dhtSensors[pin]->readTemperature();

        if (isnan(h) || isnan(t)) {
            // ss->printToAll("Failed to read from DHT sensor!");
            return false;
        }

        temperature = t;
        humidity = h;
        return true;
    }
    return false; // Sensor not found or initialized
}

bool ControlService::getPIRState(int pin, bool &motionDetected)
{
    // For a typical PIR sensor, HIGH indicates motion detected.
    motionDetected = (digitalRead(pin) == HIGH);
    return true;
}

/// @brief Constructor (Modified to call setupPWM and initialize DHT)
ControlService::ControlService(SerialService *ss) : ss(ss) { // Use initializer list
    // Initialize SerialService pointer
    this->ss = ss; // No longer needed with initializer list

    // Initialize PWM setup using LEDC
    setupPWM();

    // Declare devices
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "1bd59658-ba07-4520-b2c3-6cc7df314d4c", 18, OUTPUT, PIN_TYPE_LED);
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "ee72372d-253b-4775-85e4-9ff851a343a0", 19, OUTPUT, PIN_TYPE_LED);
    declarePin("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "891647d0-e5a8-4f02-bfce-a17facfa6e5c", 5, OUTPUT, PIN_TYPE_FAN); // Fan on pin 5 (PWM)
    // Declare DHT11 sensor - Example: DHT11 on pin 4
    declarePin("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "9e569c3f-afed-41de-9758-99a7be8ce3d7", 4, INPUT_PULLUP, PIN_TYPE_DHT11); // DHT11 on pin 4
    // Declare PIR sensor - Example: PIR sensor on pin 34
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "31d0f257-2fbc-443e-8fbb-f066de81debd", 34, INPUT, PIN_TYPE_PIR); // PIR sensor on pin 15


    configurePins();
    // Attach PWM channel to the fan pin *after* configuring pin modes
    ledcAttachPin(getPinValue("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "891647d0-e5a8-4f02-bfce-a17facfa6e5c"), pwmChannel); // Pin, Channel
}

/// @brief Destructor
ControlService::~ControlService() {
    // Clean up DHT sensor objects
    for (auto const& [pin, dhtPtr] : dhtSensors) {
        if (dhtPtr) {
            delete dhtPtr;
        }
    }
    dhtSensors.clear();
}


/// @brief Handles JSON commands
void ControlService::handleCommand(JsonDocument doc, JsonDocument &response) {
    const char *areaId = doc["areaId"];
    if (areaId == nullptr) {
        response["status"] = "error";
        response["message"] = "Missing 'areaId' in command";
        return;
    }

    if (!doc["devices"].is<JsonArray>()) {
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
        JsonObject deviceResponse = devicesResponse.add<JsonObject>(); // Create response object for each device
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
        }  else if (strcmp(function_name, "getReadings") == 0) { // New function to get sensor readings
            int pin = getPinValue(areaId, device_id);
            if (pin == -1) {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Invalid device ID '") + device_id + "' or area '" + areaId + "' for sensor readings";
                continue;
            }

            DeviceType type = getDeviceType(areaId, device_id);
            if (type == PIN_TYPE_DHT11) {
                float temperature = 0.0;
                float humidity = 0.0;
                if (getDHT11Readings(pin, temperature, humidity)) {
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("Readings for DHT11 sensor '") + device_id + "'";
                    deviceResponse["temperature_celsius"] = temperature;
                    deviceResponse["humidity_percent"] = humidity;
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Failed to read from DHT11 sensor '") + device_id + "'";
                }
            } else {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Device '") + device_id + "' is not a sensor or not supported for readings";
            }
        }
        else {
            // ss->printToAll("Unknown function '%s' for device '%s'", function_name, device_id);
            deviceResponse["status"] = "error";
            deviceResponse["message"] = "Unknown function!";
        }
    }
}

/// @brief Gets all sensor data in JSON format for all areas
/// @return A JSON string containing sensor data for all areas
String ControlService::getAllSensorDataJson() {
    DynamicJsonDocument responseDoc(2048);  // Adjust size as needed
    responseDoc["status"] = "success";
    responseDoc["message"] = "Sensor data retrieved successfully for all areas";
    JsonArray areasArray = responseDoc.createNestedArray("areas");

    for (const auto &areaPair : areaDevicesMap) {
        JsonObject areaObject = areasArray.createNestedObject();
        areaObject["areaId"] = areaPair.first;
        JsonArray sensorsArray = areaObject.createNestedArray("sensors");

        for (const auto &devicePair : areaPair.second) {
            const DeviceEntry &deviceEntry = devicePair.second;
            // Handle DHT11 sensor data
            if (deviceEntry.type == PIN_TYPE_DHT11) {
                JsonObject sensorObject = sensorsArray.createNestedObject();
                sensorObject["deviceId"] = deviceEntry.deviceId;
                sensorObject["type"] = "DHT11";
                float temperature = 0.0, humidity = 0.0;
                if (getDHT11Readings(deviceEntry.value, temperature, humidity)) {
                    sensorObject["status"] = "success";
                    sensorObject["temperature_celsius"] = temperature;
                    sensorObject["humidity_percent"] = humidity;
                } else {
                    sensorObject["status"] = "error";
                    sensorObject["message"] = "Failed to read sensor data";
                }
            }
            // Handle PIR sensor data
            else if (deviceEntry.type == PIN_TYPE_PIR) {
                JsonObject sensorObject = sensorsArray.createNestedObject();
                sensorObject["deviceId"] = deviceEntry.deviceId;
                sensorObject["type"] = "PIR";
                bool motionDetected = false;
                if (getPIRState(deviceEntry.value, motionDetected)) {
                    sensorObject["status"] = "success";
                    sensorObject["motion_detected"] = motionDetected;
                } else {
                    sensorObject["status"] = "error";
                    sensorObject["message"] = "Failed to read PIR sensor";
                }
            }
            // Handle other device types as needed…
        }
    }

    String jsonString;
    serializeJson(responseDoc, jsonString);
    return jsonString;
}
