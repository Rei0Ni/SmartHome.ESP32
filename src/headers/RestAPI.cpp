#include <RestAPI.h>
#include <Update.h>

// inline void RestAPI::onOTAStart()
// {
//     // Log when OTA has started
//     ss->printToSerial("OTA update started!");
//     // <Add your own code here>
// }

void RestAPI::onOTAProgress(size_t current, size_t final)
{
    // Log every 1 second and calculate percentage
    if (millis() - ota_progress_millis > 1000)
    {
        ota_progress_millis = millis();

        if (final > 0) { // Avoid division by zero if final size is unknown initially
            int percentage = (current * 100) / final;
            ss->printToSerial("OTA Progress: %d%% (%u/%u bytes)\n", percentage, current, final);
        } else {
            ss->printToSerial("OTA Progress: %u bytes (Final size unknown yet)\n", current);
        }
    }
}

// inline void RestAPI::onOTAEnd(bool success)
// {
//     // Log when OTA has finished
//     if (success)
//     {
//         ss->printToSerial("OTA update finished successfully!");
//         delay(1000);
//     }
//     else
//     {
//         ss->printToSerial("There was an error during OTA update!");
//     }
//     // <Add your own code here>
// }

void RestAPI::handleOTAUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
    if (!index) {
        Serial.printf("OTA Update Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  // Start OTA process
            Update.printError(Serial);
            request->send(500, "text/plain", "OTA Begin Failed!");
            return;
        }
        ota_progress_millis = millis(); // Initialize the timer here at the start of OTA
    }

    if (Update.write(data, len) != len) {  // Write firmware chunk
        Update.printError(Serial);
    } else {
        onOTAProgress(Update.progress(), Update.size()); // Call onOTAProgress after successful write
    }

    if (final) {  // Finalize update
        if (Update.end(true)) {
            Serial.println("OTA Update Success! Restarting...");
            request->send(200, "text/plain", "OTA Update Successful! Restarting...");
            delay(100);
            ESP.restart();
        } else {
            Update.printError(Serial);
            request->send(500, "text/plain", "OTA Update Failed!");
        }
    }
}

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
    server->on("/api/command/send", HTTP_POST, [this](AsyncWebServerRequest *request) {},
               nullptr, // onUpload function: not used here
               [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               { this->commandOnBody(request, data, len, index, total); });

    server->on("/update", HTTP_POST, [](AsyncWebServerRequest* request) {},
        [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
            this->handleOTAUpload(request, filename, index, data, len, final);
        },
        nullptr
    );

    // TODO: Remove in Production (Test Purposes)
    server->on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Received test request!");
        request->send(200, "text/plain", "Test OK");
    });

    server->on("/update-test", HTTP_GET, [](AsyncWebServerRequest *request){
        if (Update.begin(UPDATE_SIZE_UNKNOWN)) {
            request->send(200, "text/plain", "Update.begin() worked!");
        } else {
            request->send(500, "text/plain", "Update.begin() failed!");
        }
    });

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