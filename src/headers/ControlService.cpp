#include <ControlService.h>

#ifndef _PREFERENCES_H_
#include <Preferences.h>
#endif

using namespace std;


bool ControlService::toggle(int pin, int state)
{
    try
    {
        digitalWrite(pin, state);
        return true;
    }
    catch(const exception& e)
    {
        sh->printToAll("Error while toggling device state: %S", e.what());
        return false;
    }
    
}

void ControlService::declarePin(const char *name, int value, int mode)
{
    PinEntry entry = {name, value, mode};
    pinTable.push_back(entry);
}

int ControlService::getPinValue(const char *pinName)
{
    for (int i = 0; i < pinTable.size(); i++)
    {
        if (strcmp(pinTable[i].name, pinName) == 0)
        {
            return pinTable[i].value;
        }
    }
    // Return -1 if the pin name is not found
    return -1;
}

void ControlService::configurePins()
{
    for (int i = 0; i < pinTable.size(); i++) {
        pinMode(pinTable[i].value, pinTable[i].mode);
    }
}

ControlService::ControlService(SerialService *sh)
{
    this->sh = sh;
    declarePin("LED_1", 18, GPIO_MODE_INPUT_OUTPUT);
    declarePin("LED_2", 19, GPIO_MODE_INPUT_OUTPUT);
}

ControlService::~ControlService()
{
}

void ControlService::handleCommand(JsonDocument doc, JsonDocument &response) {
    // Extract the area
    const char *area = doc["area"];

    // Loop through devices
    for (JsonObject device : doc["devices"].as<JsonArray>()) {
        const char *device_id = device["device_id"];

        // Loop through functions
        for (JsonObject function : device["functions"].as<JsonArray>()) {
            const char *function_name = function["function_name"];

            // Extract parameters
            JsonObject parameters = function["parameters"];
            
            // Execute the command
            if (strcmp(function_name, "toggle") == 0) {
                const char *state = parameters["state"];
                if (state) {
                    int pin = getPinValue(device_id);
                    int level = !strcmp(state, "on") ? HIGH : LOW;
                    this->toggle(pin, level);
                }
                response["status"] = "Success";
                response["message"] = String("Toggled to ") + state;
            } else {
                Serial.println("  Unknown function!");
                response["status"] = "error";
                response["message"] = "Unknown function!";
            }
        }
    }
}