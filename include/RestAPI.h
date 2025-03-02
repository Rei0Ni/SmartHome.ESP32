#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// #ifndef ElegantOTA_h
// #include <ElegantOTA.h>
// #endif

#ifndef SerialHelper_h
#include <SerialService.h>
#endif

#ifndef ControlService_h
#include <ControlService.h>
#endif

#ifndef RestAPI_h
#define RestAPI_h

class SerialService;
class ControlService;

class RestAPI
{
private:
    AsyncWebServer *server;
    SerialService *ss;
    ControlService *cs;

    JsonDocument jsonDocument;
    char buffer[1024];

    unsigned long ota_progress_millis = 0;

    // void onOTAStart();
    void onOTAProgress(size_t current, size_t final);
    // void onOTAEnd(bool success);

    void handleOTAUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);

public:
    RestAPI(ControlService *cs, SerialService *ss, AsyncWebServer *server);
    ~RestAPI();
    void setupApi();
    void commandOnRequest(AsyncWebServerRequest *request);
    void commandOnBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
};

#endif