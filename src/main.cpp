#include <Arduino.h>
#include <WiFi.h>

#include <WifiManagerService.h>
#include <SerialService.h>
#include <ControlService.h>
#include <RestAPI.h>
#include <MessageQueueService.h>

AsyncWebServer server(2826);
WifiManagerService wm;
SerialService ss(&wm);
ControlService cs(&ss);
RestAPI RestApi(&cs, &ss, &server);
MessageQueueService mq(&cs, &ss, 5000, "192.168.1.9", 1883, "sensor_data", "mqttuser", "P@ssw0rd", [] { return cs.getAllSensorDataJson(); });

void setup() {
  ss.Initialize(115200, &server);
  wm.Initialize("P@ssw0rd");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Start with the LED off

  RestApi.setupApi();
  mq.start();
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