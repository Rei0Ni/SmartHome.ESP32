/**
 * @file WifiManagerService.h
 * @brief WiFi connection manager using WiFiManager library
 * 
 * Provides simplified WiFi management with fallback to configuration portal
 * when connection fails. Handles ESP32 WiFi configuration and access point management.
 */

#ifndef WifiManagerService_h
#define WifiManagerService_h

#include <WiFiManager.h>
#include "I2CLedScreen.h"

/**
 * @class WifiManagerService
 * @brief Manages WiFi connectivity and configuration portal for ESP32 devices
 * 
 * This class wraps the WiFiManager functionality to provide:
 * - Automatic connection to stored WiFi credentials
 * - Fallback configuration access point when no credentials exist
 * - Password-protected configuration portal
 * - System reset capabilities for network settings
 */
class WifiManagerService
{
private:
    WiFiManager wm; ///< Instance of the WiFiManager library
    I2CLedScreen* lcd; ///< LCD screen instance
    TaskHandle_t scrollTaskHandle = NULL;
    TaskHandle_t apCredentialsTaskHandle = NULL;
    SemaphoreHandle_t lcdMutex;

    static void scrollTask(void *pvParameters); // FreeRTOS task function
    static void apCredentialsTask(void *pvParameters);

    /**
     * @brief Stops both configuration and web portals
     * @private
     * 
     * Called automatically after successful WiFi connection to disable
     * the configuration portals and free resources
     */
    void stopPortal();

public:
    WifiManagerService(I2CLedScreen* lcd);
    ~WifiManagerService();

    /**
     * @brief Initialize WiFi connection with fallback portal
     * @param apPassword Password for the configuration access point (minimum 8 characters)
     * 
     * Behavior flow:
     * 1. Attempt connection using stored credentials
     * 2. If no stored credentials or connection fails:
     *    - Launches password-protected configuration portal
     *    - Saves new credentials if configured via portal
     * 3. Restarts ESP if portal times out without configuration
     */
    void Initialize(const char* apPassword);

    /**
     * @brief Reset all stored WiFi settings and restart device
     * 
     * Performs complete WiFi configuration reset:
     * - Erases all stored SSID/password combinations
     * - Restarts ESP to apply changes
     * - Device will enter configuration portal on next boot
     */
    void resetAndRestart();
    void displayIPandPort(uint16_t port);
    void displayAPCredentials(const char *ssid, const char *password, int &ssidScrollPos, int &passwordScrollPos);
    void displayScrollingText(uint8_t row, const String &text, int &scrollPos);
};

struct TaskParams
{
    WifiManagerService *service;
    uint16_t port;
};

struct APCredentialsParams {
    WifiManagerService *service;
    const char *ssid;
    const char *password;
};

#endif