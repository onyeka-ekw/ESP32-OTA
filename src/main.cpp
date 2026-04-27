#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <EEPROM.h>
#include "ota_manager.h"



const int LED_PIN = 2; // Built-in LED on ESP32

// Timing constants
const unsigned long WIFI_CONNECT_TIMEOUT = 10000;
const unsigned long UPDATE_CHECK_INTERVAL = 3600000; // 1 hour in milliseconds

// EEPROM constants for persistent user settings
#define EEPROM_SIZE 128
#define VERSION_ADDR 0
#define VERSION_SIZE 16
#define USER_SSID_ADDR 16
#define USER_SSID_SIZE 32
#define USER_PASSWORD_ADDR 48
#define USER_PASSWORD_SIZE 64
#define CONFIG_FLAGS_ADDR 112
#define CONFIG_FLAGS_SIZE 1
#define CONFIGURED_FLAG 0xAA

// Global variables
unsigned long lastUpdateCheck = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;
String userSSID = "";
String userPassword = "";

// Objects
OTAManager otaManager;

// Function declarations
void connectToWiFi();
void checkForUpdates();
void setupUserCredentials();
void enterSetupMode();
bool isUserConfigured();
void loadUserCredentials(String &ssid, String &password);
void saveUserCredentials(const String &ssid, const String &password);
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Get stored firmware version from EEPROM
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
    
    // Check if user has configured credentials
    if (!isUserConfigured()) {
        Serial.println("First time setup - configuring WiFi credentials...");
        enterSetupMode();
    } else {
        // Load user credentials from EEPROM
        loadUserCredentials(userSSID, userPassword);
        Serial.println("Loaded user credentials from EEPROM");
    }
    
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
    
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLedBlink = currentTime;
    Serial.println("LED State Version 5: " + String(ledState ? "ON" : "OFF"));
    
    delay(500);
}

void connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    Serial.println("Target SSID: " + userSSID);
    
    WiFi.begin(userSSID.c_str(), userPassword.c_str());
    
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
        // Blink 3 times to indicate connection failure
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
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

bool isUserConfigured() {
    byte configFlag = EEPROM.read(CONFIG_FLAGS_ADDR);
    return configFlag == CONFIGURED_FLAG;
}

void loadUserCredentials(String &ssid, String &password) {
    // Load SSID
    ssid = "";
    for (int i = 0; i < USER_SSID_SIZE; i++) {
        char c = EEPROM.read(USER_SSID_ADDR + i);
        if (c == 0) break;
        ssid += c;
    }
    
    // Load Password
    password = "";
    for (int i = 0; i < USER_PASSWORD_SIZE; i++) {
        char c = EEPROM.read(USER_PASSWORD_ADDR + i);
        if (c == 0) break;
        password += c;
    }
}

void saveUserCredentials(const String &ssid, const String &password) {
    // Save SSID
    for (int i = 0; i < USER_SSID_SIZE; i++) {
        if (i < ssid.length()) {
            EEPROM.write(USER_SSID_ADDR + i, ssid.charAt(i));
        } else {
            EEPROM.write(USER_SSID_ADDR + i, 0);
        }
    }
    
    // Save Password
    for (int i = 0; i < USER_PASSWORD_SIZE; i++) {
        if (i < password.length()) {
            EEPROM.write(USER_PASSWORD_ADDR + i, password.charAt(i));
        } else {
            EEPROM.write(USER_PASSWORD_ADDR + i, 0);
        }
    }
    
    // Mark as configured
    EEPROM.write(CONFIG_FLAGS_ADDR, CONFIGURED_FLAG);
    EEPROM.commit();
    
    Serial.println("User credentials saved to EEPROM");
}

void enterSetupMode() {
    Serial.println("\n=== ESP32 OTA Setup Mode ===");
    Serial.println("Please configure your WiFi credentials");
    Serial.println("Format: SSID,PASSWORD (e.g., MyWiFi,MyPassword)");
    Serial.println("Type 'reset' to clear credentials and restart setup");
    Serial.println("Type 'skip' to use factory credentials");
    Serial.println("Waiting for input...\n");
    
    String input = "";
    bool setupComplete = false;
    
    while (!setupComplete) {
        if (Serial.available()) {
            input = Serial.readStringUntil('\n');
            input.trim();
            
            if (input.equalsIgnoreCase("reset")) {
                // Clear configuration flag
                EEPROM.write(CONFIG_FLAGS_ADDR, 0x00);
                EEPROM.commit();
                Serial.println("Credentials reset. Restarting...");
                delay(1000);
                ESP.restart();
            }
            else if (input.equalsIgnoreCase("skip")) {
                Serial.println("Cannot skip - WiFi credentials are required for OTA updates");
                Serial.println("Please enter your WiFi credentials in format: SSID,PASSWORD");
            }
            else if (input.length() > 0 && input.indexOf(',') > 0) {
                // Parse SSID and Password
                int commaIndex = input.indexOf(',');
                String ssid = input.substring(0, commaIndex);
                String password = input.substring(commaIndex + 1);
                
                ssid.trim();
                password.trim();
                
                if (ssid.length() > 0 && password.length() > 0) {
                    Serial.println("Setting credentials:");
                    Serial.println("SSID: " + ssid);
                    Serial.println("Password: " + String(password.length()) + " characters");
                    
                    // Save to EEPROM
                    saveUserCredentials(ssid, password);
                    userSSID = ssid;
                    userPassword = password;
                    setupComplete = true;
                    
                    Serial.println("Setup complete! Credentials saved.");
                } else {
                    Serial.println("Invalid format. Please use: SSID,PASSWORD");
                }
            }
            else {
                Serial.println("Invalid input. Please use format: SSID,PASSWORD");
                Serial.println("Or type 'reset' to clear, 'skip' to use factory");
            }
        }
        delay(100);
    }
}
