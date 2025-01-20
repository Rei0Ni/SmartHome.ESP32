#ifndef WiFiManager_h
#include <WiFiManager.h>
#endif

#ifndef WifiManagerService_h
#define WifiManagerServcie_h
// WifiManagerService
class WifiManagerService
{
private:
    WiFiManager wm;
    void stopPortal();
public:
    WifiManagerService(/* args */);
    ~WifiManagerService();

    void Initialize(const char* apPassword);
    void resetAndRestart();
    
};

#endif