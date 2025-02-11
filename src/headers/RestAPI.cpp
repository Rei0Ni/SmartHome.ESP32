#include <RestAPI.h>

// inline void RestAPI::onOTAStart()
// {
//     // Log when OTA has started
//     serial.printToSerial("OTA update started!");
//     // <Add your own code here>
// }

// inline void RestAPI::onOTAProgress(size_t current, size_t final)
// {
//     // Log every 1 second
//     if (millis() - ota_progress_millis > 1000)
//     {
//         ota_progress_millis = millis();
//         serial.printToSerial("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
//     }
// }

// inline void RestAPI::onOTAEnd(bool success)
// {
//     // Log when OTA has finished
//     if (success)
//     {
//         serial.printToSerial("OTA update finished successfully!");
//         delay(1000);
//     }
//     else
//     {
//         serial.printToSerial("There was an error during OTA update!");
//     }
//     // <Add your own code here>
// }

/// @brief Constructor for RestAPI
/// @param cs Pointer to the ControlService instance
/// @param ss Pointer to the SerialService instance
/// @param server Pointer to the AsyncWebServer instance
RestAPI::RestAPI(ControlService *cs, SerialService *ss, AsyncWebServer *server)
{
    this->server = server;
    this->ss = ss;
    this->cs = cs;
}

/// @brief Destructor for RestAPI
RestAPI::~RestAPI()
{
}

/// @brief Sets up the API endpoints
void RestAPI::setupApi()
{
    server->on("/api/command/send", HTTP_POST, [this](AsyncWebServerRequest *request)
               { this->commandOnRequest(request); },
               nullptr, // onUpload function: not used here
               [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               { this->commandOnBody(request, data, len, index, total); });

    // ElegantOTA.begin(server);
    // ElegantOTA.setAuth("OTAdmin", "P@ssw0rd");
    // ElegantOTA.onStart([this]()
    //                    { this->onOTAStart(); });
    // ElegantOTA.onProgress([this](size_t current, size_t final)
    //                       { this->onOTAProgress(current, final); });
    // ElegantOTA.onEnd([this](bool success)
    //                  { this->onOTAEnd(success); });

    server->begin();
}

/// @brief Handles the command request
/// @param request Pointer to the AsyncWebServerRequest instance
void RestAPI::commandOnRequest(AsyncWebServerRequest *request)
{

    // // Send a success response
    // request->send(200, "application/json", "{\"status\":\"success\"}");
    //  request->send(200, "application/json", "{\"status\":\"success\"}");
}

/// @brief Handles the body of the command request
/// @param request Pointer to the AsyncWebServerRequest instance
/// @param data Pointer to the data received
/// @param len Length of the data received
/// @param index Index of the data chunk
/// @param total Total size of the data
inline void RestAPI::commandOnBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    // Check request method
    if (request->method() != 2)
    {
        request->send(405, "application/json", "{\"status\":\"error\",\"message\":\"Method Not Allowed\"}");
        return;
    }

    JsonDocument response;
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, data);
    if (error)
    {
        response["status"] = "error";
        response["message"] = "Failed to parse JSON!";
        String stringResponse;
        serializeJson(response, stringResponse);
        request->send(400, "application/json", stringResponse);
        return;
    }

    cs->handleCommand(doc, response);

    String stringResponse;
    serializeJson(response, stringResponse);
    request->send(200, "application/json", stringResponse);
}