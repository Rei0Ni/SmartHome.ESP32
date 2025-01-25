#ifndef WifiManagerService_h
#define WifiManagerService_h

#include <WiFiManager.h>

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