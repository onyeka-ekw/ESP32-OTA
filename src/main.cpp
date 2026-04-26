#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "ota_manager.h"

// Try to include config.h, but provide fallbacks for GitHub Actions
#ifdef __has_include
  #if __has_include("config.h")
    #include "config.h"
  #else
    // Fallback configuration for GitHub Actions
    #define WIFI_SSID "GITHUB_ACTIONS_BUILD"
    #define WIFI_PASSWORD "GITHUB_ACTIONS_BUILD"
    #define OTA_SERVER "esp32-ota-349412601154-us-east-1-an.s3.amazonaws.com"
    #define VERSION_CHECK_ENDPOINT "https://esp32-ota-349412601154-us-east-1-an.s3.amazonaws.com/version.json"
  #endif
#else
  // Fallback configuration for older compilers
  #define WIFI_SSID "GITHUB_ACTIONS_BUILD"
  #define WIFI_PASSWORD "GITHUB_ACTIONS_BUILD"
  #define OTA_SERVER "esp32-ota-349412601154-us-east-1-an.s3.amazonaws.com"
  #define VERSION_CHECK_ENDPOINT "https://esp32-ota-349412601154-us-east-1-an.s3.amazonaws.com/version.json"
#endif

// Function declarations
void connectToWiFi();
void checkForUpdates();

// LED configuration
const int LED_PIN = 2; // Built-in LED on ESP32

// Timing constants
const unsigned long WIFI_CONNECT_TIMEOUT = 10000;
const unsigned long UPDATE_CHECK_INTERVAL = 3600000; // 1 hour in milliseconds
const unsigned long LED_BLINK_INTERVAL = 2000;

// Global variables
unsigned long lastUpdateCheck = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;

// Objects
OTAManager otaManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 OTA Firmware Starting...");
    Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Connect to WiFi
    connectToWiFi();
    
    // Check for updates on startup
    checkForUpdates();
    
    lastUpdateCheck = millis();
    lastLedBlink = millis();
    
    Serial.println("Setup completed. Running main loop...");
}

void loop() {
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Reconnecting...");
        connectToWiFi();
    }
    
    // Check for updates every hour
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateCheck >= UPDATE_CHECK_INTERVAL) {
        Serial.println("Checking for updates...");
        checkForUpdates();
        lastUpdateCheck = currentTime;
    }
    
    // Blink LED to indicate device is running
    if (currentTime - lastLedBlink >= LED_BLINK_INTERVAL) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        lastLedBlink = currentTime;
        Serial.println("LED State: " + String(ledState ? "ON" : "OFF"));
    }
    
    delay(100);
}

void connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.println("IP address: " + WiFi.localIP().toString());
        digitalWrite(LED_PIN, HIGH); // Turn on LED to indicate WiFi connected
    } else {
        Serial.println("\nFailed to connect to WiFi");
        // Blink 3 times to indicate connection failure
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(200);
            digitalWrite(LED_PIN, LOW);
            delay(200);
        }
    }
}

void checkForUpdates() {
    Serial.println("Checking for firmware updates...");
    
    if (otaManager.checkForUpdate(VERSION_CHECK_ENDPOINT)) {
        Serial.println("Update available! Starting OTA update...");
        if (otaManager.performUpdate(OTA_SERVER)) {
            Serial.println("OTA update completed successfully!");
            Serial.println("Restarting device...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("OTA update failed!");
            // Blink 5 times to indicate update failure
            for (int i = 0; i < 5; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
            }
        }
    } else {
        Serial.println("No updates available.");
    }
}
