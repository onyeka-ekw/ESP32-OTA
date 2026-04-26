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

bool OTAManager::performUpdate(const char* firmwareUrl) {
    String fullUrl = String(firmwareUrl);
    if (!fullUrl.startsWith("http://") && !fullUrl.startsWith("https://")) {
        fullUrl = "https://" + fullUrl;
    }
    Serial.println("Starting OTA update from: " + fullUrl);
    
    // Simple test - just try to begin update with a reasonable size
    size_t firmwareSize = 1000000; // 1MB should be enough for most firmware
    
    if (!Update.begin(firmwareSize)) {
        Serial.println("Update begin failed: " + String(Update.errorString()));
        return false;
    }
    
    Serial.println("Update begun successfully, starting download...");
    
    // Download and write firmware
    if (!downloadFirmware(fullUrl.c_str())) {
        Serial.println("Firmware download failed");
        Update.abort();
        return false;
    }
    
    // Complete the update
    if (!Update.end(true)) { // true to verify the written firmware
        Serial.println("Update end failed: " + String(Update.errorString()));
        return false;
    }
    
    Serial.println("OTA update completed successfully!");
    return true;
}

String OTAManager::getCurrentVersion() {
    return currentVersion;
}

String OTAManager::getAvailableVersion() {
    return availableVersion;
}

const char* OTAManager::getFirmwareUrl() {
    return firmwareUrl.c_str();
}

void OTAManager::setCurrentVersion(const String& version) {
    currentVersion = version;
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
        Serial.println("Parsed firmware URL: " + firmwareUrl);
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
    Serial.println("Starting firmware download...");
    
    HTTPClient http;
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(60000); // 60 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int totalSize = http.getSize();
        
        WiFiClient* stream = http.getStreamPtr();
        
        // Enhanced download loop with percentage display
        uint8_t buffer[256]; // Very small buffer to prevent stack issues
        int written = 0;
        int lastProgress = 0;
        
        while (http.connected() && (written < totalSize || totalSize == -1)) {
            if (stream->available()) {
                int bytesRead = stream->readBytes(buffer, min(256, stream->available()));
                
                if (bytesRead > 0) {
                    if (Update.write(buffer, bytesRead) != bytesRead) {
                        Serial.println("Write failed: " + String(Update.errorString()));
                        http.end();
                        return false;
                    }
                    written += bytesRead;
                    
                    // Show percentage progress every 5KB or when progress changes
                    if (totalSize > 0) {
                        int progress = (written * 100) / totalSize;
                        if (progress != lastProgress || written % 5000 == 0) {
                            Serial.println("\rProgress: " + String(progress) + "% (" + String(written) + "/" + String(totalSize) + " bytes)");
                            lastProgress = progress;
                        }
                    } else {
                        // For unknown size, show bytes downloaded
                        if (written % 50000 == 0) {
                            Serial.print("\rDownloaded: " + String(written) + " bytes");
                        }
                    }
                }
            }
            delay(5); // Small delay
        }
        
        // Show final completion
        if (totalSize > 0) {
            Serial.println("\rProgress: 100% (" + String(written) + "/" + String(totalSize) + " bytes) - Complete!");
        } else {
            Serial.println("\nDownload completed: " + String(written) + " bytes");
        }
        http.end();
        
        return true;
    } else {
        Serial.println("HTTP error: " + String(httpCode));
        http.end();
        return false;
    }
}

bool OTAManager::writeFirmware(uint8_t* data, size_t length) {
    return Update.write(data, length) == length;
}

bool OTAManager::verifyFirmware() {
    return Update.isFinished();
}
