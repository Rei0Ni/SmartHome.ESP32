#include <Arduino.h>
#include <WiFi.h>

#include <WifiManagerService.h>
#include <SerialService.h>
#include <ControlService.h>
#include <RestAPI.h>
#include <MessageQueueService.h>
#include "I2CLedScreen.h"

#define SCREEN_I2C_ADDRESS 0x27 // Change according to your screen

AsyncWebServer server(2826);
I2CLedScreen screen(SCREEN_I2C_ADDRESS, 16, 2);
WifiManagerService wm(&screen);
SerialService ss(&wm);
ControlService cs(&ss);
RestAPI RestApi(&cs, &ss, &server);
MessageQueueService mq(&cs, &ss, 5000, "192.168.1.9", 1883, "sensor_data", "mqttuser", "P@ssw0rd", []
                       { return cs.getAllSensorDataJson(); });
// LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address based on I2C scanner

void setup()
{
  ss.Initialize(115200, &server);
  wm.Initialize("P@ssw0rd");
  // screen.begin();
  // screen.displayText("Hello ESP32!");

  // Serial.println("\nI2C Scanner: Scanning for devices...");
    
  // for (byte address = 1; address < 127; address++) {
  //     Wire.beginTransmission(address);
  //     if (Wire.endTransmission() == 0) {
  //         Serial.print("Found device at: 0x");
  //         Serial.println(address, HEX);
  //         delay(10);
  //     }
  // }

  // lcd.init();
  // lcd.backlight();
  // lcd.setCursor(0, 0);
  // lcd.print("Hello, ESP32!");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Start with the LED off

  RestApi.setupApi();
  mq.start();
}

void loop()
{
  // int touchValue = touchRead(4);

  // ss.printToSerial("Touch value: %d\n", touchValue);
  // if (touchValue > 100) {
  //   ss.printToSerial("Touch value: %d\n", touchValue);
  //   wm.resetAndRestart();
  // }
  // mandatory for webserial
  // ss.loop();
}