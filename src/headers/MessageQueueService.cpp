#include "MessageQueueService.h"
#include <ArduinoJson.h>
#include "SerialService.h"

void MessageQueueService::taskFunction(void* pvParameters) {
    MessageQueueService* service = static_cast<MessageQueueService*>(pvParameters);
    while (service->isRunning) {
        service->publishMessage();
        vTaskDelay(pdMS_TO_TICKS(service->publishIntervalMs));
    }
    vTaskDelete(NULL);
}

MessageQueueService::MessageQueueService(ControlService* cs, SerialService* ss, int intervalMs, const char* broker, int port, const char* topic, const char* username, const char* password, std::function<String()> dataProvider) // Modified constructor
    : controlService(cs), serialService(ss), mqttClient(espClient), publishIntervalMs(intervalMs), mqttBroker(broker), mqttPort(port), mqttTopic(topic), mqttUsername(username), mqttPassword(password), taskHandle(NULL), isRunning(false), dataProviderFunction(dataProvider) { // Initialize dataProviderFunction
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setBufferSize(16384);
}

MessageQueueService::~MessageQueueService() {
    stop();
}

void MessageQueueService::start() {
    if (!isRunning) {
        isRunning = true;
         BaseType_t taskCreationResult = xTaskCreatePinnedToCore(
            taskFunction,
            "MessageQueueServiceTask",
            16384 * 2,  // **Increase stack size to 32768 (or even higher, like 65536 for testing)**
            this,
            1,
            &taskHandle,
            APP_CPU_NUM
        );
        if (taskCreationResult != pdPASS) {
            Serial.print("Error creating MessageQueueService task!\n");
            isRunning = false;
        } else {
            Serial.print("MessageQueueService task started.\n");
        }
    } else {
        Serial.print("MessageQueueService task is already running.\n");
    }
}

void MessageQueueService::stop() {
    if (isRunning) {
        isRunning = false;
        if (taskHandle != NULL) {
            taskHandle = NULL;
        }
        Serial.print("MessageQueueService task stopping.");
    } else {
        Serial.println("MessageQueueService task is not running.");
    }
}

void MessageQueueService::publishMessage() {
    if (!mqttClient.connected()) {
        Serial.print("MQTT Client not connected before publish, calling reconnect()\n");
        reconnect();
        if (!mqttClient.connected()) {
            Serial.print("MQTT still not connected after reconnect attempt. Publish skipped.\n");
            return;
        }
    }

    String jsonData = dataProviderFunction(); // Comment out dataProvider
    // String jsonData = "{\"test\":\"short\"}"; // **Very short, minimal JSON**
    // Serial.print("jsonData to publish: ");
    // Serial.print(jsonData + "\n");

    // **Debug: Check connection state just before publish**
    // Serial.print("MQTT Client connected state before publish: ");
    // Serial.print(mqttClient.connected() + "\n"); // Will print 1 (true) if connected, 0 (false) if not

    if (mqttClient.publish(mqttTopic, jsonData.c_str())) {
        // Serial.print("Published to MQTT Topic: ");
        // Serial.print(mqttTopic);
        // Serial.print("\n");
        // Serial.print("Published to MQTT: ");
        // Serial.print(jsonData + "\n");
    } else {
        Serial.print("MQTT publish failed!\n");
    }
}

void MessageQueueService::reconnect() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...\n");
        // **Capture the connect status code**
        int connectStatus = mqttClient.connect("ESP32Client", mqttUsername, mqttPassword);
        if (connectStatus) { // connect() returns 0 for success, non-zero for failure
            Serial.print("Connected to MQTT Broker!\n");
            // mqttClient.subscribe(mqttTopic);  // Subscribe if necessary
        } else {
            Serial.print("Failed, connectStatus=");
            Serial.print(connectStatus); // **Print the connectStatus code**
            Serial.print(", mqttClient.state()="); // Also keep printing state for comparison
            Serial.print(mqttClient.state());
            Serial.print("\n Trying again in 5 seconds...\n");
            delay(5000);  // Wait before retrying
        }
    }
}
