#include <Arduino.h>
// #include <Adafruit_Sensor.h>
#include <WiFi.h>



#include <WifiManagerService.h>
#include <SerialService.h>

// will be moved to RestApi later
#include <ESPAsyncWebServer.h>


AsyncWebServer server(2826);
WifiManagerService wm;
SerialService ss(&wm);

void setup() {
  ss.Initialize(115200, &server);
  wm.Initialize("P@ssw0rd");

  // will be moved to RestApi later
  server.begin();
}

void loop() {
  ss.printToAll("Hello World");
  delay(1000);
  
  // mandatory for webserial
  ss.loop();
}