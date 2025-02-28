#include <Arduino.h>
#include <WiFi.h>

#include <WifiManagerService.h>
#include <SerialService.h>
#include <ControlService.h>
#include <RestAPI.h>

AsyncWebServer server(2826);
WifiManagerService wm;
SerialService ss(&wm);
ControlService cs(&ss);
RestAPI RestApi(&cs, &ss, &server);

void setup() {
  ss.Initialize(115200, &server);
  wm.Initialize("P@ssw0rd");

  RestApi.setupApi();
}

void loop() {
  // int touchValue = touchRead(4);

  // ss.printToSerial("Touch value: %d\n", touchValue);
  // if (touchValue > 100) {
  //   ss.printToSerial("Touch value: %d\n", touchValue);
  //   wm.resetAndRestart();
  // }
  // mandatory for webserial
  // ss.loop();
}