#include <Arduino.h>
// #include <Adafruit_Sensor.h>
#include <WiFi.h>



#ifndef WifiManagerService_h
#include <WifiManagerService.h>
#endif

// will be moved to RestApi later
#ifndef _ESPAsyncWebServer_H_
#include <ESPAsyncWebServer.h>
#endif

AsyncWebServer server(2826);
WifiManagerService wm;

void setup() {
  wm.Initialize("P@ssw0rd");

  // will be moved to RestApi later
  server.begin();
}

void loop() {
  
}