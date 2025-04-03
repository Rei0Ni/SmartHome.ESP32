#include "ControlService.h"
#include "SerialService.h"
#include <cstring>

#include <Arduino.h> // Make sure to include Arduino.h for ESP32 functions

// Include DHT sensor library - Make sure you have installed the DHT sensor library in Arduino IDE Library Manager
#include <DHT.h>

using namespace std;

/// @brief Gets the next available PWM channel.
int ControlService::getNextAvailablePwmChannel() {
    if (nextPwmChannel < maxPwmChannels) return nextPwmChannel++;
    return -1; // No channels left
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

            // Find the PWM channel associated with this fan pin
            for (auto const& areaPair : areaDevicesMap) {
                for (auto const& devicePair : areaPair.second) {
                    if (devicePair.second.value == pin && devicePair.second.type == PIN_TYPE_FAN && devicePair.second.pwmChannel != -1) {
                        // Use ledcWrite with the mapped PWM value and the correct channel
                        ss->printToAll("Setting fan on pin %d (channel %d) to %d%% -> PWM %d\n", 
                                        pin, devicePair.second.pwmChannel, speedPercentage, pwmValue);
                        ledcWrite(devicePair.second.pwmChannel, pwmValue);
                        fanSpeeds[pin] = speedPercentage; // Store the fan speed
                        if (speedPercentage > 0) {
                            lastActiveFanSpeeds[pin] = speedPercentage;
                            ss->printToAll("Stored last active speed for pin %d: %d%%\n", pin, speedPercentage);
                        }
                        ss->printToAll("Fan speed set to %d%% (PWM value: %d) on pin %d, channel %d\n", speedPercentage, pwmValue, pin, devicePair.second.pwmChannel); // Debug print
                        return true;
                    }
                }
            }
            ss->printToAll("Error: Fan pin %d not configured for PWM.\n", pin); // Debug print
            return false;
        } else {
            ss->printToAll("Error: Invalid fan speed percentage %d. Must be between 0 and 100.\n", speedPercentage); // Debug print
            return false;
        }
    } catch (const exception& e) {
        ss->printToAll("Error while controlling fan speed: %s\n", e.what()); // Debug print
        return false;
    }
}

/// @brief Controls LED brightness using PWM
bool ControlService::controlLedBrightness(int pin, int brightness) {
    try {
        brightness = constrain(brightness, 0, 100);
        int pwmValue = ::map(brightness, 0, 100, 0, 255);

        // Find the PWM channel associated with this LED pin
        for (auto const& areaPair : areaDevicesMap) {
            for (auto const& devicePair : areaPair.second) {
                if (devicePair.second.value == pin && devicePair.second.type == PIN_TYPE_LED && devicePair.second.pwmChannel != -1) {
                    ss->printToAll("Setting LED on pin %d (channel %d) to %d%% -> PWM %d\n", 
                                    pin, devicePair.second.pwmChannel, brightness, pwmValue);
                    ledcWrite(devicePair.second.pwmChannel, pwmValue);
                    ledBrightness[pin] = brightness;
                    // Only update the last *active* brightness if it's being set to non-zero
                    if (brightness > 0) {
                        lastActiveLedBrightness[pin] = brightness;
                        ss->printToAll("Stored last active brightness for pin %d: %d%%\n", pin, brightness);
                    }
                    ss->printToAll("LED brightness set to %d%% (PWM value: %d) on pin %d, channel %d\n", brightness, pwmValue, pin, devicePair.second.pwmChannel);
                    return true;
                }
            }
        }
        ss->printToAll("Error: LED pin %d not configured for PWM.\n", pin);
        return false;
    } catch (const exception& e) {
        ss->printToAll("Error while controlling LED brightness: %s\n", e.what());
        return false;
    }
}

/// @brief Declares a pin for a device
void ControlService::declarePin(const char *areaId, const char *deviceId, int value, int mode, DeviceType type) {
    int pwmChannel = -1;

    if (type == PIN_TYPE_FAN || type == PIN_TYPE_LED) {
        pwmChannel = getNextAvailablePwmChannel();
        if (pwmChannel == -1) {
            Serial.println("No PWM channels available!");
            return;
        }
        
        // Configure PWM frequency based on device type
        int freq = (type == PIN_TYPE_FAN) ? 25000 : 5000; // 25 kHz for fan, 5 kHz for LED
        ledcSetup(pwmChannel, freq, pwmResolutionBits);
        ledcAttachPin(value, pwmChannel);
        ledcWrite(pwmChannel, 0); // Initialize to 0% duty cycle
    } else if (type == PIN_TYPE_DHT11) {
        DHT* dht = new DHT(value, DHT11);
        dht->begin();
        dhtSensors[value] = dht;
        ss->printToAll("Initialized DHT11 sensor on pin %d\n", value);
    }

    areaDevicesMap[areaId][deviceId] = {deviceId, value, mode, type, pwmChannel};
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
    // this->ss = ss;

    // Initialize PWM setup (not really needed as setup is done per pin now)
    // setupPWM();

    // // Declare devices and assign PWM channels
    // declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "1bd59658-ba07-4520-b2c3-6cc7df314d4c", 18, OUTPUT, PIN_TYPE_LED);
    // declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "ee72372d-253b-4775-85e4-9ff851a343a0", 19, OUTPUT, PIN_TYPE_LED);
    // declarePin("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "891647d0-e5a8-4f02-bfce-a17facfa6e5c", 5, OUTPUT, PIN_TYPE_FAN); // Fan on pin 5 (PWM)
    // // Declare DHT11 sensor - Example: DHT11 on pin 4
    // declarePin("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "9e569c3f-afed-41de-9758-99a7be8ce3d7", 4, INPUT_PULLUP, PIN_TYPE_DHT11); // DHT11 on pin 4
    // // Declare PIR sensor - Example: PIR sensor on pin 34
    // declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "31d0f257-2fbc-443e-8fbb-f066de81debd", 34, INPUT, PIN_TYPE_PIR); // PIR sensor on pin 15


    // configurePins();
    // No need to attach PWM channel here, it's done during declarePin
    // ledcAttachPin(getPinValue("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "891647d0-e5a8-4f02-bfce-a17facfa6e5c"), pwmChannel); // Pin, Channel
}

/// @brief Destructor
ControlService::~ControlService() {
    // Clean up DHT sensor objects
    for (auto const& sensor : dhtSensors) {
        int pin = sensor.first;
        DHT* dhtPtr = sensor.second;
        if (dhtPtr) {
            delete dhtPtr;
        }
    }
    dhtSensors.clear();
}

/// @brief Initializes the ControlService
/// @details This function sets up the PWM, declares devices, configures pins, and initializes sensors.
void ControlService::Initialize()
{
    // setupPWM();
    declareDevices(); // Moved from constructor
    // configurePins();
}


/// @brief Handles JSON commands
void ControlService::handleCommand(JsonDocument doc, JsonDocument &response) {
    const char *areaId = doc["areaId"];
    if (areaId == nullptr) {
        response["status"] = "error";
        response["message"] = "Missing 'areaId' in command";
        Serial.println("Error: Missing 'areaId' in command"); // Debug print
        return;
    }

    if (!doc["devices"].is<JsonArray>()) {
        response["status"] = "error";
        response["message"] = "Missing or invalid 'devices' array in command";
        Serial.println("Error: Missing or invalid 'devices' array in command"); // Debug print
        return;
    }

    if (!areaDevicesMap.count(areaId)) {
        response["status"] = "error";
        response["message"] = String("Area '") + areaId + "' not found";
        ss->printToAll("Error: Area '%s' not found\n", areaId); // Debug print
        return;
    }

    JsonArray devicesResponse = response["devices"].to<JsonArray>(); // Create an array for device responses

    for (JsonObject device : doc["devices"].as<JsonArray>()) {
        const char *device_id = device["deviceId"];
        const char *function_name = device["function"];
        JsonObject deviceResponse = devicesResponse.add<JsonObject>(); // Create response object for each device
        deviceResponse["deviceId"] = device_id; // Add deviceId to device response

        if (device_id == nullptr || function_name == nullptr) {
            deviceResponse["status"] = "error";
            deviceResponse["message"] = "Missing 'deviceId' or 'function' in device command";
            Serial.println("Error: Missing 'deviceId' or 'function' in device command"); // Debug print
            continue;
        }

        int pin = getPinValue(areaId, device_id);
        DeviceType type = getDeviceType(areaId, device_id);

        if (pin == -1) {
            deviceResponse["status"] = "error";
            deviceResponse["message"] = String("Invalid device ID '") + device_id + "' or area '" + areaId + "'";
            ss->printToAll("Error: Invalid device ID '%s' or area '%s'\n", device_id, areaId); // Debug print
            continue;
        }

        if (strcmp(function_name, "toggle") == 0) {
            if (type == PIN_TYPE_FAN) {
                JsonObject parameters = device["parameters"];
                if (!parameters.containsKey("state")) {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = "Missing 'state' parameter for 'toggle' function";
                    Serial.println("Error: Missing 'state' parameter for 'toggle' function"); // Debug print
                    continue;
                }

                bool state = parameters["state"];
                int targetSpeedPercentage = 0; // Default to OFF


                if (state == true) { // ---> TOGGLE ON <---
                    // Retrieve the last active speed, default to 100 if not found
                    if (lastActiveFanSpeeds.count(pin)) {
                        targetSpeedPercentage = lastActiveFanSpeeds[pin];
                        // Ensure the retrieved speed isn't somehow 0, use default if it is
                        if (targetSpeedPercentage <= 0) {
                             targetSpeedPercentage = 100; // Or another sensible default ON speed
                             ss->printToAll("Warning: Last active speed for pin %d was 0, using default %d%%\n", pin, targetSpeedPercentage);
                        } else {
                             ss->printToAll("Restoring last active speed for pin %d: %d%%\n", pin, targetSpeedPercentage);
                        }
                    } else {
                        targetSpeedPercentage = 100; // Default speed if never turned on before
                        ss->printToAll("No last active speed found for pin %d, using default %d%%\n", pin, targetSpeedPercentage);
                    }
                } else { // ---> TOGGLE OFF <---
                    targetSpeedPercentage = 0;
                    ss->printToAll("Toggling fan OFF for pin %d\n", pin);
                }

                if (this->controlFanSpeed(pin, targetSpeedPercentage)) {
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("Toggled fan '") + device_id + "' to state " + (state ? "on" : "off");
                    deviceResponse["power_state"] = (targetSpeedPercentage > 0) ? "on" : "off"; // Update based on actual target speed
                    deviceResponse["fan_speed"] = targetSpeedPercentage;       // Add fan_speed to response
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Toggle failed for fan '") + device_id + "'";
                }
            } else if (type == PIN_TYPE_LED) {
                JsonObject parameters = device["parameters"];
                if (!parameters.containsKey("state")) {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = "Missing 'state' parameter for 'toggle' function";
                    Serial.println("Error: Missing 'state' parameter for 'toggle' function"); // Debug print
                    continue;
                }

                bool state = parameters["state"];
                int targetBrightness = 0; // Default to OFF

                if (state == true) { // ---> TOGGLE ON <---
                    // Retrieve the last active brightness, default to 100 if not found
                    if (lastActiveLedBrightness.count(pin)) {
                        targetBrightness = lastActiveLedBrightness[pin];
                         // Ensure the retrieved brightness isn't somehow 0, use default if it is
                        if (targetBrightness <= 0) {
                             targetBrightness = 100; // Or another sensible default ON brightness
                             ss->printToAll("Warning: Last active brightness for pin %d was 0, using default %d%%\n", pin, targetBrightness);
                        } else {
                             ss->printToAll("Restoring last active brightness for pin %d: %d%%\n", pin, targetBrightness);
                        }
                    } else {
                        targetBrightness = 100; // Default brightness if never turned on before
                        ss->printToAll("No last active brightness found for pin %d, using default %d%%\n", pin, targetBrightness);
                    }
                } else { // ---> TOGGLE OFF <---
                    targetBrightness = 0;
                    ss->printToAll("Toggling LED OFF for pin %d\n", pin);
                }

                // Use setbrightness function to control LED
                if (controlLedBrightness(pin, targetBrightness)) {
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("Toggled LED '") + device_id + "' to state " + (state ? "on" : "off");
                    deviceResponse["power_state"] = (targetBrightness > 0) ? "on" : "off"; // Update based on actual target brightness
                    deviceResponse["brightness"] = targetBrightness; // Report the brightness being set
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Toggle failed for LED '") + device_id + "'";
                }
            } else {
                JsonObject parameters = device["parameters"];
                if (!parameters.containsKey("state")) {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = "Missing 'state' parameter for 'toggle' function";
                    Serial.println("Error: Missing 'state' parameter for 'toggle' function"); // Debug print
                    continue;
                }

                bool state = parameters["state"];
                int level = (state == true) ? HIGH : LOW;

                if (this->toggle(pin, level)) {
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("Toggled device '") + device_id + "' to state " + (state ? "on" : "off");
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Toggle failed for device '") + device_id + "'";
                }
            }
        } else if (strcmp(function_name, "setspeed") == 0) {
            if (type == PIN_TYPE_FAN) {
                JsonObject parameters = device["parameters"];
                if (!parameters.containsKey("speed")) {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = "Missing 'speed' parameter for 'setspeed' function";
                    Serial.println("Error: Missing 'speed' parameter for 'setspeed' function"); // Debug print
                    continue;
                }

                int speedPercentage = parameters["speed"]; // **Expect speed as an integer percentage (0-100) now**

                if (this->controlFanSpeed(pin, speedPercentage)) { // **Call controlFanSpeed with speedPercentage**
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("Set fan '") + device_id + "' speed to " + speedPercentage + "%"; // Update message to show percentage
                    deviceResponse["fan_speed"] = speedPercentage;                                                     // Add fan_speed to response
                    deviceResponse["power_state"] = (speedPercentage > 0) ? "on" : "off";                             // Add power_state to response
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Failed to set speed for fan '") + device_id + "'";
                }
            } else {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Function 'setspeed' is not applicable for device type");
            }
        } else if (strcmp(function_name, "setbrightness") == 0) {
            if (type == PIN_TYPE_LED) {
                JsonObject parameters = device["parameters"];
                if (!parameters.containsKey("brightness")) {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = "Missing 'brightness' parameter for 'setbrightness' function";
                    Serial.println("Error: Missing 'brightness' parameter for 'setbrightness' function"); // Debug print
                    continue;
                }

                int brightness = parameters["brightness"];

                if (this->controlLedBrightness(pin, brightness)) {
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("Set LED '") + device_id + "' brightness to " + brightness;
                    deviceResponse["brightness"] = brightness;
                    deviceResponse["power_state"] = (brightness > 0) ? "on" : "off";
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Failed to set brightness for LED '") + device_id + "'";
                }
            } else {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Function 'setbrightness' is not applicable for device type");
            }
        } else if (strcmp(function_name, "getReadings") == 0) { // New function to get sensor readings
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
            } else if (type == PIN_TYPE_PIR) {
                bool motionDetected = false;
                if (getPIRState(pin, motionDetected)) {
                    deviceResponse["status"] = "success";
                    deviceResponse["message"] = String("State for PIR sensor '") + device_id + "'";
                    deviceResponse["motion_detected"] = motionDetected;
                } else {
                    deviceResponse["status"] = "error";
                    deviceResponse["message"] = String("Failed to read PIR sensor '") + device_id + "'";
                }
            } else {
                deviceResponse["status"] = "error";
                deviceResponse["message"] = String("Device '") + device_id + "' is not a sensor or not supported for readings";
            }
        } else {
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

void ControlService::declareDevices() {
    // Declare devices and assign PWM channels
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "1bd59658-ba07-4520-b2c3-6cc7df314d4c", 18, OUTPUT, PIN_TYPE_LED);
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "ee72372d-253b-4775-85e4-9ff851a343a0", 19, OUTPUT, PIN_TYPE_LED);
    declarePin("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "891647d0-e5a8-4f02-bfce-a17facfa6e5c", 5, OUTPUT, PIN_TYPE_FAN); // Fan on pin 5 (PWM)
    // Declare DHT11 sensor - Example: DHT11 on pin 4
    declarePin("8dca5204-a0ac-4ec6-83f7-c3b8acdf6e5b", "9e569c3f-afed-41de-9758-99a7be8ce3d7", 4, INPUT_PULLUP, PIN_TYPE_DHT11); // DHT11 on pin 4
    // Declare PIR sensor - Example: PIR sensor on pin 34
    declarePin("94c4dab3-19bf-448a-90d5-b9b00ec0cda0", "31d0f257-2fbc-443e-8fbb-f066de81debd", 34, INPUT, PIN_TYPE_PIR); // PIR sensor on pin 15

    controlLedBrightness(18, 0);
    controlLedBrightness(19, 0);
    controlFanSpeed(5, 0);
}