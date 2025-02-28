#include "WifiManagerService.h"  // Full definition of WifiManagerService
#include <SerialService.h>

SerialService::SerialService(WifiManagerService *wm)
{
    this->wm = wm;
}

SerialService::~SerialService()
{
}

/// @brief initializes the physical serial and web serial ports
/// @param baud speed atwhich data is sent
/// @param server the server at which the webserial endpoint will be available
void SerialService::Initialize(int baud, AsyncWebServer *server)
{
    Serial.begin(baud);
    // WebSerial.begin(server);
    // WebSerial.onMessage([this](uint8_t *data, size_t len) {
    //     this->recvMsg(data, len);
    // });
}

/// @brief prints to both physical serial and webserial
/// @param format the string to format the message after
/// @param args arguments that will be formatted into the message
void SerialService::printToAll(const char *format, ...)
{
    char buffer[1024]; // Fixed-size stack buffer
    va_list args;

    va_start(args, format); // Initialize the argument list
    int len = vsnprintf(buffer, sizeof(buffer), format, args); // Format the string
    va_end(args);

    if (len < 0) {
        Serial.println("Error: Formatting failed!");
        WebSerial.println("Error: Formatting failed!");
    } else if (len >= sizeof(buffer)) {
        Serial.println("Warning: Log message truncated!");
        WebSerial.println("Warning: Log message truncated!");
    }

    Serial.println(buffer);
    WebSerial.println(buffer); // Print the formatted string
}

/// @brief prints to physical serial
/// @param format the string to format the message after
/// @param args arguments that will be formatted into the message
void SerialService::printToSerial(const char *format, ...)
{
    char buffer[1024]; // Fixed-size stack buffer
    va_list args;

    va_start(args, format); // Initialize the argument list
    int len = vsnprintf(buffer, sizeof(buffer), format, args); // Format the string
    va_end(args);

    if (len < 0) {
        Serial.println("Error: Formatting failed!");
    } else if (len >= sizeof(buffer)) {
        Serial.println("Warning: Log message truncated!");
    }

    Serial.println(buffer);// print to physical serial
}

/// @brief prints to webserial
/// @param format the string to format the message after
/// @param args arguments that will be formatted into the message
void SerialService::printToWebSerial(const char *format, ...)
{
    char buffer[1024]; // Fixed-size stack buffer
    va_list args;

    va_start(args, format); // Initialize the argument list
    int len = vsnprintf(buffer, sizeof(buffer), format, args); // Format the string
    va_end(args);

    if (len < 0) {
        WebSerial.println("Error: Formatting failed!");
    } else if (len >= sizeof(buffer)) {
        WebSerial.println("Warning: Log message truncated!");
    }

    WebSerial.println(buffer); // Print the formatted string
}

void SerialService::loop()
{
    WebSerial.loop();
}

/// @brief handles command sent through the webserial 
/// @param command string that may contain a command to be executed
void SerialService::commandHandler(String command)
{
    if(command == "/help") {
        printToAll("Available commands:");
        printToAll("/wifi reset - Reset network settings");
        printToAll("/system info - Show device information");
    }
    else if(command == "/system info") {
        printToAll("Chip ID: %d\nChip Revision: %d\nFree heap: %d bytes", ESP.getChipModel(), ESP.getChipRevision(), ESP.getFreeHeap());
    }else if(command == "/wifi reset"){
        wm->resetAndRestart();
    }else if(command == "/wifi info"){
        printToAll("IP Address: %s", WiFi.localIP().toString());
    }
    else {
        printToAll("Unknown command: %s", command.c_str());
    }
}

/// @brief handles the message comming from webserial
/// @param data data recieved
/// @param len length of data
void SerialService::recvMsg(uint8_t *data, size_t len)
{
  String d = "";
  for(size_t i = 0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
  commandHandler(d);
}