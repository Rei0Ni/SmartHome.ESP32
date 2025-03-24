#include <WifiManagerService.h>


WifiManagerService::WifiManagerService(I2CLedScreen *lcd) : lcd(lcd)
{
    lcdMutex = xSemaphoreCreateMutex();
}

WifiManagerService::~WifiManagerService()
{
    if (scrollTaskHandle != NULL) {
        vTaskDelete(scrollTaskHandle);
    }
    if (apCredentialsTaskHandle != NULL) {
        vTaskDelete(apCredentialsTaskHandle);
    }
    if(lcdMutex != NULL){
        vSemaphoreDelete(lcdMutex);
    }
}

/// @brief initialize wifi connection
/// @param apPassword password to secure the AccessPoint if wifi connection fails
void WifiManagerService::Initialize(const char *apPassword)
{
    // Always start displaying AP credentials immediately
    Serial.println("Starting AP credentials display...");

    // Ensure previous AP credentials task is not running
    if (apCredentialsTaskHandle != NULL) {
        vTaskDelete(apCredentialsTaskHandle);
        apCredentialsTaskHandle = NULL;
    }

    lcd->begin();
    delay(100); // Small delay to ensure LCD initializes properly

    APCredentialsParams *params = new APCredentialsParams{this, wm.getDefaultAPName().c_str(), apPassword};

    xTaskCreatePinnedToCore(
        WifiManagerService::apCredentialsTask,
        "AP_Credentials_Task",
        4096,
        params,
        1,
        &apCredentialsTaskHandle,
        1);

    // Now attempt to connect to WiFi
    bool res = this->wm.autoConnect(wm.getDefaultAPName().c_str(), apPassword);

    // check if wifi connected
    if (!res) {
        Serial.println("Failed to connect");

        ESP.restart();
    }

    // if you get here you have connected to the WiFi
    Serial.println("connected...😊");

    // Stop displaying AP credentials now that WiFi is connected
    if (apCredentialsTaskHandle != NULL) {
        vTaskDelete(apCredentialsTaskHandle);
        apCredentialsTaskHandle = NULL;
    }

    // Stop the portal
    this->stopPortal();

    // Display IP on LCD
    uint16_t port = 2826; // Change this if using a different port
    displayIPandPort(port);
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

void WifiManagerService::displayIPandPort(uint16_t port) {
    lcd->begin();
    lcd->clear();

    TaskParams *params = new TaskParams{this, port};

    xTaskCreatePinnedToCore(
        WifiManagerService::scrollTask,
        "LCD_Scroll_Task",
        4096,
        params,
        1,
        &scrollTaskHandle,
        1);
}

void WifiManagerService::scrollTask(void *pvParameters) {
    TaskParams *params = (TaskParams *)pvParameters;
    WifiManagerService *service = params->service;
    uint16_t port = params->port;
    delete params;

    String ipText = "IP: " + WiFi.localIP().toString();
    String portText = "Port: " + String(port);

    if (ipText.length() > 16) {
        ipText += "                ";
    }

    int ipScrollPos = 0;
    int portScrollPos = 0;

    while (true) {
        service->displayScrollingText(0, ipText, ipScrollPos);
        service->displayScrollingText(1, portText, portScrollPos);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void WifiManagerService::apCredentialsTask(void *pvParameters) {
    APCredentialsParams *params = (APCredentialsParams *)pvParameters;
    WifiManagerService *service = params->service;
    const char *ssid = params->ssid;
    const char *password = params->password;
    delete params;

    int ssidScrollPos = 0;
    int passwordScrollPos = 0;

    while (true) {
        service->displayAPCredentials(ssid, password, ssidScrollPos, passwordScrollPos);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void WifiManagerService::displayAPCredentials(const char *ssid, const char *password, int &ssidScrollPos, int &passwordScrollPos) {
    if (xSemaphoreTake(lcdMutex, portMAX_DELAY) == pdTRUE) {
        String ssidText = "SSID: " + String(ssid);
        String passwordText = "Pass: " + String(password);

        if (ssidText.length() > 16) {
            ssidText += "                ";
        }

        if (passwordText.length() > 16) {
            passwordText += "                ";
        }

        lcd->displayScrollingText(0, ssidText, ssidScrollPos);
        lcd->displayScrollingText(1, passwordText, passwordScrollPos);

        xSemaphoreGive(lcdMutex);
    }
}

void WifiManagerService::displayScrollingText(uint8_t row, const String &text, int &scrollPos) {
    if (xSemaphoreTake(lcdMutex, portMAX_DELAY) == pdTRUE) {
        lcd->displayScrollingText(row, text, scrollPos);
        xSemaphoreGive(lcdMutex);
    }
}