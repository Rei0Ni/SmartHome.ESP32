#ifndef MessageQueueService_h
#define MessageQueueService_h

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "ControlService.h"

class MessageQueueService
{
private:
    ControlService *controlService;
    SerialService *serialService;
    WiFiClient espClient;
    PubSubClient mqttClient;
    int publishIntervalMs;
    const char *mqttTopic;
    const char *mqttBroker;
    int mqttPort;
    const char *mqttUsername;
    const char *mqttPassword;
    TaskHandle_t taskHandle;
    bool isRunning;
    std::function<String()> dataProviderFunction;

    static void taskFunction(void *pvParameters);
    void publishMessage();
    void reconnect();

public:
    MessageQueueService(ControlService *cs, SerialService *ss, int intervalMs, const char *broker, int port, const char *topic, const char *username, const char *password, std::function<String()> dataProvider);
    void start();
    void stop();
    ~MessageQueueService();
};

#endif // MessageQueueService_h
