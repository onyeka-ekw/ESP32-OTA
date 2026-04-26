#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>
#include "ota_manager.h"

// Fallback configuration for GitHub Actions (always defined)
#define WIFI_SSID "HeatSense"
#define WIFI_PASSWORD "HeatSense"
#define OTA_SERVER "esp32-ota-349412601154-us-east-1-an.s3.amazonaws.com"
#define VERSION_CHECK_ENDPOINT "https://esp32-ota-349412601154-us-east-1-an.s3.amazonaws.com/version.json"

// Try to override with local config.h if it exists
#ifdef __has_include
  #if __has_include("config.h")
    #include "config.h"
  #endif
#endif

// Function declarations
void connectToWiFi();
void checkForUpdates();
void scanWiFiNetworks();

// LED configuration
const int LED_PIN = 2; // Built-in LED on ESP32

// Timing constants
const unsigned long WIFI_CONNECT_TIMEOUT = 10000;
const unsigned long UPDATE_CHECK_INTERVAL = 3600000; // 1 hour in milliseconds
const unsigned long LED_BLINK_INTERVAL = 2000;

// EEPROM constants
#define EEPROM_SIZE 64
#define VERSION_ADDR 0

// Global variables
unsigned long lastUpdateCheck = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;

// Objects
OTAManager otaManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Get stored version from EEPROM
    String storedVersion = "";
    bool isValidVersion = true;
    
    for (int i = 0; i < 16; i++) {
        char c = EEPROM.read(VERSION_ADDR + i);
        if (c == 0) break; // End of string
        if (c < 32 || c > 126) { // Check for non-printable characters
            isValidVersion = false;
            break;
        }
        storedVersion += c;
    }
    
    // If no valid version stored, use compiled version
    if (storedVersion.length() == 0 || !isValidVersion) {
        storedVersion = String(FIRMWARE_VERSION);
        Serial.println("EEPROM not initialized, using compiled version: " + storedVersion);
        
        // Clear and save initial version to EEPROM
        for (int i = 0; i < 16; i++) {
            EEPROM.write(VERSION_ADDR + i, 0);
        }
        for (int i = 0; i < storedVersion.length(); i++) {
            EEPROM.write(VERSION_ADDR + i, storedVersion.charAt(i));
        }
        EEPROM.write(VERSION_ADDR + storedVersion.length(), 0); // Null terminator
        EEPROM.commit();
        Serial.println("Initialized EEPROM with version: " + storedVersion);
    }
    
    Serial.println("ESP32 OTA Firmware Starting...");
    Serial.println("Compiled Version: " + String(FIRMWARE_VERSION));
    Serial.println("Stored Version: " + storedVersion);
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Scan WiFi networks first
    Serial.println("Scanning available WiFi networks...");
    scanWiFiNetworks();
    
    // Connect to WiFi
    connectToWiFi();
    
    // Set current version for OTA comparison
    otaManager.setCurrentVersion(storedVersion);
    
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
        Serial.println("LED State Version 2: " + String(ledState ? "ON" : "OFF"));
    }
    
    delay(1);
}

void connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    Serial.println("Target SSID: " + String(WIFI_SSID));
    Serial.println("Password length: " + String(strlen(WIFI_PASSWORD)));
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
        
        // Show WiFi status during connection
        if (millis() - startTime % 2000 == 0) {
            Serial.println("\nStatus: " + String(WiFi.status()));
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.println("IP address: " + WiFi.localIP().toString());
        digitalWrite(LED_PIN, HIGH); // Turn on LED to indicate WiFi connected
    } else {
        Serial.println("\nFailed to connect to WiFi");
        Serial.println("Final status: " + String(WiFi.status()));
        Serial.println("Possible causes:");
        Serial.println("- Wrong password");
        Serial.println("- Network security type not supported");
        Serial.println("- MAC address filtering");
        
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
        if (otaManager.performUpdate(otaManager.getFirmwareUrl())) {
            // Save new version to EEPROM before restart
            String newVersion = otaManager.getAvailableVersion();
            
            Serial.println("OTA update completed successfully!");
            Serial.println("Saving new version to EEPROM: " + newVersion);
            
            // Clear old version and write new version
            for (int i = 0; i < 16; i++) {
                EEPROM.write(VERSION_ADDR + i, 0);
            }
            for (int i = 0; i < newVersion.length(); i++) {
                EEPROM.write(VERSION_ADDR + i, newVersion.charAt(i));
            }
            EEPROM.write(VERSION_ADDR + newVersion.length(), 0); // Null terminator
            
            bool commitSuccess = EEPROM.commit();
            Serial.println("EEPROM commit " + String(commitSuccess ? "successful" : "failed"));
            
            // Verify the version was saved
            String verifyVersion = "";
            for (int i = 0; i < 16; i++) {
                char c = EEPROM.read(VERSION_ADDR + i);
                if (c == 0) break;
                verifyVersion += c;
            }
            Serial.println("Verified EEPROM version: " + verifyVersion);
            
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

void scanWiFiNetworks() {
    Serial.println("Starting WiFi scan...");
    
    int n = WiFi.scanNetworks();
    Serial.println("Scan complete!");
    
    if (n == 0) {
        Serial.println("No WiFi networks found");
    } else {
        Serial.print(String(n) + " networks found:\n");
        for (int i = 0; i < n; ++i) {
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print("dBm)");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [OPEN]" : " [SECURED]");
            delay(10);
        }
    }
    Serial.println("");
}
