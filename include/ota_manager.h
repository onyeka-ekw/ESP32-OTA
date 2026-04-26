#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

class OTAManager {
public:
    OTAManager();
    
    // Check if an update is available
    bool checkForUpdate(const char* versionUrl);
    
    // Perform the OTA update
    bool performUpdate(const char* server);
    
    // Get current firmware version
    String getCurrentVersion();
    
    // Get available firmware version
    String getAvailableVersion();

private:
    String currentVersion;
    String availableVersion;
    String firmwareUrl;
    
    // Parse version information from JSON
    bool parseVersionInfo(const String& jsonPayload);
    
    // Download firmware file
    bool downloadFirmware(const char* url);
    
    // Write firmware to flash
    bool writeFirmware(uint8_t* data, size_t length);
    
    // Verify firmware integrity
    bool verifyFirmware();
};

#endif // OTA_MANAGER_H
