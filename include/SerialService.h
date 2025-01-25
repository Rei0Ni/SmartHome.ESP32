#ifndef WebSerial_h
#include <WebSerial.h>
#endif

#ifndef SerialService_h
#define SerialService_h
// SerialService

class WifiManagerService;  // Forward declaration

class SerialService
{
private:
    WifiManagerService *wm;
    void commandHandler(String command);
    void recvMsg(uint8_t *data, size_t len);
public:
    SerialService(WifiManagerService *wm);
    ~SerialService();
    void Initialize(int baud, AsyncWebServer *server);
    void printToAll(const char *format, ...);
    void printToSerial(const char *format, ...);
    void printToWebSerial(const char *format, ...);
    void loop();
};;

#endif