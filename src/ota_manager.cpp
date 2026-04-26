#include "ota_manager.h"

OTAManager::OTAManager() {
    currentVersion = String(FIRMWARE_VERSION);
}

bool OTAManager::checkForUpdate(const char* versionUrl) {
    HTTPClient http;
    
    Serial.println("Checking version from: " + String(versionUrl));
    
    http.begin(versionUrl);
    http.setTimeout(10000); // 10 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Version info received: " + payload);
        
        http.end();
        
        if (parseVersionInfo(payload)) {
            Serial.println("Current version: " + currentVersion);
            Serial.println("Available version: " + availableVersion);
            
            // Simple version comparison (assumes semantic versioning)
            if (availableVersion != currentVersion) {
                Serial.println("Update available!");
                return true;
            }
        }
    } else {
        Serial.println("Failed to check version. HTTP code: " + String(httpCode));
        http.end();
        return false;
    }
    
    return false;
}

bool OTAManager::performUpdate(const char* server) {
    if (firmwareUrl.length() == 0) {
        Serial.println("No firmware URL available");
        return false;
    }
    
    Serial.println("Starting OTA update from: " + firmwareUrl);
    
    // Begin update process
    size_t firmwareSize = 0;
    
    if (!Update.begin(firmwareSize)) {
        Serial.println("Update begin failed");
        return false;
    }
    
    // Download and write firmware
    if (!downloadFirmware(firmwareUrl.c_str())) {
        Serial.println("Firmware download failed");
        Update.abort();
        return false;
    }
    
    // Complete the update
    if (!Update.end(true)) { // true to verify the written firmware
        Serial.println("Update end failed: " + String(Update.errorString()));
        return false;
    }
    
    Serial.println("OTA update completed successfully");
    return true;
}

String OTAManager::getCurrentVersion() {
    return currentVersion;
}

String OTAManager::getAvailableVersion() {
    return availableVersion;
}

bool OTAManager::parseVersionInfo(const String& jsonPayload) {
    DynamicJsonDocument doc(1024);
    
    DeserializationError error = deserializeJson(doc, jsonPayload);
    
    if (error) {
        Serial.println("JSON parsing failed: " + String(error.c_str()));
        return false;
    }
    
    // Extract version information
    if (doc.containsKey("version")) {
        availableVersion = doc["version"].as<String>();
    }
    
    if (doc.containsKey("firmware_url")) {
        firmwareUrl = doc["firmware_url"].as<String>();
    }
    
    if (doc.containsKey("required")) {
        bool required = doc["required"].as<bool>();
        if (required) {
            Serial.println("This is a required update!");
        }
    }
    
    return true;
}

bool OTAManager::downloadFirmware(const char* url) {
    HTTPClient http;
    
    http.begin(url);
    http.setTimeout(30000); // 30 second timeout for download
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("HTTP GET failed, code: " + String(httpCode));
        http.end();
        return false;
    }
    
    int totalSize = http.getSize();
    int remaining = totalSize;
    int written = 0;
    
    Serial.println("Firmware size: " + String(totalSize) + " bytes");
    
    WiFiClient* stream = http.getStreamPtr();
    
    while (remaining > 0) {
        int available = stream->available();
        
        if (available) {
            uint8_t buffer[1024];
            int readSize = min((size_t)available, sizeof(buffer));
            
            int bytesRead = stream->readBytes(buffer, readSize);
            
            if (Update.write(buffer, bytesRead) != bytesRead) {
                Serial.println("Write failed during update");
                http.end();
                return false;
            }
            
            remaining -= bytesRead;
            written += bytesRead;
            
            // Show progress
            int progress = (written * 100) / totalSize;
            Serial.print("\rProgress: " + String(progress) + "%");
        }
        
        delay(1);
    }
    
    Serial.println("\nDownload completed");
    http.end();
    
    return true;
}

bool OTAManager::writeFirmware(uint8_t* data, size_t length) {
    return Update.write(data, length) == length;
}

bool OTAManager::verifyFirmware() {
    return Update.isFinished();
}
