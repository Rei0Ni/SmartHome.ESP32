#include <WifiManagerService.h>

WifiManagerService::WifiManagerService(/* args */)
{
}

WifiManagerService::~WifiManagerService()
{
}

/// @brief initialize wifi connection
/// @param apPassword password to secure the AccessPoint if wifi connection fails
void WifiManagerService::Initialize(const char* apPassword){
    bool res = this->wm.autoConnect(wm.getDefaultAPName().c_str(), apPassword);

    // check if wifi connected
    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 

    //if you get here you have connected to the WiFi  
    Serial.println("connected...😊");
    this->stopPortal();
}

/// @brief reset wificonfigurations and restart ESP32
void WifiManagerService::resetAndRestart()
{
    this->wm.resetSettings();
    ESP.restart();
}

/// @brief stop WifiManager Portal if open
void WifiManagerService::stopPortal()
{
    this->wm.stopConfigPortal();
    this->wm.stopWebPortal();
}