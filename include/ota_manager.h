#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

class OTAManager {
public:
    OTAManager();
    
    bool checkForUpdate(const char* versionUrl);
    bool performUpdate(const char* firmwareUrl);
    const char* getFirmwareUrl();
    String getCurrentVersion();
    String getAvailableVersion();
    void setCurrentVersion(const String& version);

private:
    String currentVersion;
    String availableVersion;
    String firmwareUrl;
    
    // Parse version information from JSON
    bool parseVersionInfo(const String& jsonPayload);
    bool downloadFirmware(const char* url);
    bool writeFirmware(uint8_t* data, size_t length);
    bool verifyFirmware();
};

#endif // OTA_MANAGER_H
