#include "ESPOTAUpdater.h"

ESPOTAUpdater::ESPOTAUpdater(const char* githubRepo, int currentFirmwareVersion) 
    : _githubRepo(githubRepo), 
      _currentFirmwareVersion(currentFirmwareVersion),
      _lastUpdateCheck(0),
      _updateInterval(5 * 60 * 1000UL), // 5 minutes default
      _autoUpdateEnabled(true),
      _updateAvailableCallback(nullptr),
      _updateProgressCallback(nullptr),
      _updateCompleteCallback(nullptr) {
    
    // Auto-detect board type from compile-time defines
    #ifdef BOARD_TYPE
        _boardType = String(BOARD_TYPE);
    #else
        _boardType = "UNKNOWN";
    #endif
}

void ESPOTAUpdater::setBoardType(const String& boardType) {
    _boardType = boardType;
}

String ESPOTAUpdater::getBoardType() {
    return _boardType;
}

void ESPOTAUpdater::setUpdateInterval(unsigned long intervalMs) {
    _updateInterval = intervalMs;
}

bool ESPOTAUpdater::shouldCheckForUpdates(unsigned long currentTime) {
    return (currentTime - _lastUpdateCheck) > _updateInterval;
}

void ESPOTAUpdater::updateLastCheckTime(unsigned long currentTime) {
    _lastUpdateCheck = currentTime;
}

void ESPOTAUpdater::enableAutoUpdate(bool enabled) {
    _autoUpdateEnabled = enabled;
}

bool ESPOTAUpdater::isAutoUpdateEnabled() {
    return _autoUpdateEnabled;
}

void ESPOTAUpdater::setUpdateAvailableCallback(UpdateAvailableCallback callback) {
    _updateAvailableCallback = callback;
}

void ESPOTAUpdater::setUpdateProgressCallback(UpdateProgressCallback callback) {
    _updateProgressCallback = callback;
}

void ESPOTAUpdater::setUpdateCompleteCallback(UpdateCompleteCallback callback) {
    _updateCompleteCallback = callback;
}

void ESPOTAUpdater::checkForUpdates() {
    Serial.println("Checking for updates from GitHub releases...");
    HTTPClient http;
    
    // Use GitHub API to get latest release
    String url = "https://api.github.com/repos/" + String(_githubRepo) + "/releases/latest";
    http.begin(url);
    http.addHeader("User-Agent", "ESP32-OTA-Updater");
    
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Failed to get GitHub release info, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    // Parse the GitHub API response
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    // Extract version from tag_name
    String tagName = doc["tag_name"].as<String>();
    Serial.println("Latest release tag: " + tagName);
    
    int newVersion = parseVersionFromTag(tagName);
    if (newVersion == 0) {
        Serial.println("Could not parse version from tag");
        return;
    }

    // Find appropriate firmware binary
    JsonArray assets = doc["assets"];
    String binaryUrl = findBoardSpecificFirmware(assets);
    
    if (binaryUrl == "") {
        binaryUrl = findGenericFirmware(assets);
    }
    
    if (binaryUrl == "") {
        Serial.println("No firmware binary found in release assets");
        return;
    }
    
    Serial.printf("Current version: %d, Latest version: %d\n", _currentFirmwareVersion, newVersion);

    if (newVersion > _currentFirmwareVersion) {
        Serial.println("*** NEW FIRMWARE AVAILABLE ***");
        Serial.println("Download URL: " + binaryUrl);
        
        // Call callback if set
        if (_updateAvailableCallback) {
            _updateAvailableCallback(_currentFirmwareVersion, newVersion, binaryUrl);
        }
        
        // Auto-update if enabled
        if (_autoUpdateEnabled) {
            Serial.println("Starting OTA update...");
            performUpdate(binaryUrl.c_str());
        } else {
            Serial.println("Auto-update disabled. Manual update required.");
        }
    } else if (newVersion == _currentFirmwareVersion) {
        Serial.println("Current firmware is up to date.");
    } else {
        Serial.println("Current firmware is newer than latest release.");
    }
}

void ESPOTAUpdater::performUpdate(const char* url) {
    Serial.println("Starting OTA update process...");
    Serial.printf("Downloading from: %s\n", url);
    
    bool success = downloadAndInstallFirmware(String(url));
    
    if (_updateCompleteCallback) {
        String message = success ? "Update completed successfully" : "Update failed";
        _updateCompleteCallback(success, message);
    }
    
    if (success) {
        Serial.println("Update successful! Rebooting...");
        delay(1000);
        ESP.restart();
    }
}

int ESPOTAUpdater::parseVersionFromTag(const String& tagName) {
    int newVersion = 0;
    if (tagName.startsWith("v")) {
        String versionStr = tagName.substring(1); // Remove 'v'
        
        // Parse major.minor format (e.g., "9.1" -> 91)
        int dotIndex = versionStr.indexOf('.');
        if (dotIndex > 0) {
            int major = versionStr.substring(0, dotIndex).toInt();
            int minor = versionStr.substring(dotIndex + 1).toInt();
            newVersion = major * 10 + minor; // Convert 9.1 to 91
            Serial.printf("Parsed version: %d.%d -> %d\n", major, minor, newVersion);
        } else {
            // Fallback for old single number format
            float versionFloat = versionStr.toFloat();
            newVersion = (int)(versionFloat * 10);
            Serial.printf("Legacy version format: %f -> %d\n", versionFloat, newVersion);
        }
    }
    return newVersion;
}

String ESPOTAUpdater::findBoardSpecificFirmware(JsonArray assets) {
    // Determine board-specific filename
    String boardSpecificFile = "";
    if (_boardType == "ESP32_DEVKIT") {
        boardSpecificFile = "firmware-esp32-devkit.bin";
    } else if (_boardType == "XIAO_ESP32S3") {
        boardSpecificFile = "firmware-xiao-esp32s3.bin";
    }
    
    // Look for board-specific firmware
    if (boardSpecificFile != "") {
        for (JsonObject asset : assets) {
            String assetName = asset["name"].as<String>();
            if (assetName == boardSpecificFile) {
                Serial.println("Found board-specific firmware: " + assetName);
                return asset["browser_download_url"].as<String>();
            }
        }
    }
    
    return "";
}

String ESPOTAUpdater::findGenericFirmware(JsonArray assets) {
    for (JsonObject asset : assets) {
        String assetName = asset["name"].as<String>();
        if (assetName == "firmware.bin") {
            Serial.println("Found generic firmware: " + assetName);
            return asset["browser_download_url"].as<String>();
        }
    }
    return "";
}

bool ESPOTAUpdater::downloadAndInstallFirmware(const String& url) {
    HTTPClient http;
    http.setTimeout(30000); // 30 second timeout for large files
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);
    http.addHeader("User-Agent", "ESP32-OTA-Updater");
    
    Serial.println("Sending GET request...");
    int httpCode = http.GET();
    
    Serial.printf("HTTP response code: %d\n", httpCode);

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Failed to download binary, HTTP code: %d\n", httpCode);
        Serial.printf("Error description: %s\n", http.errorToString(httpCode).c_str());
        
        String response = http.getString();
        if (response.length() > 0) {
            Serial.printf("Response body: %s\n", response.c_str());
        }
        
        http.end();
        return false;
    }
    
    int contentLength = http.getSize();
    Serial.printf("Content length: %d bytes\n", contentLength);
    
    if (contentLength <= 0) {
        Serial.println("Content-Length header invalid or missing.");
        http.end();
        return false;
    }

    Serial.printf("Available heap before update: %d bytes\n", ESP.getFreeHeap());
    
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        Serial.printf("Not enough space to begin OTA. Required: %d bytes\n", contentLength);
        Serial.printf("Available space: %d bytes\n", Update.size());
        http.end();
        return false;
    }

    Serial.println("Starting firmware write...");
    WiFiClient& stream = http.getStream();
    
    size_t written = 0;
    size_t totalWritten = 0;
    uint8_t buffer[1024];
    
    while (totalWritten < contentLength) {
        size_t bytesToRead = min(sizeof(buffer), contentLength - totalWritten);
        size_t bytesRead = stream.readBytes(buffer, bytesToRead);
        
        if (bytesRead == 0) break;
        
        written = Update.write(buffer, bytesRead);
        if (written != bytesRead) {
            Serial.printf("Write error: expected %d, got %d\n", bytesRead, written);
            break;
        }
        
        totalWritten += written;
        
        // Call progress callback if set
        if (_updateProgressCallback) {
            _updateProgressCallback(totalWritten, contentLength);
        }
        
        // Print progress every 10KB
        if (totalWritten % 10240 == 0 || totalWritten == contentLength) {
            Serial.printf("Progress: %d/%d bytes (%.1f%%)\n", 
                         totalWritten, contentLength, 
                         (float)totalWritten / contentLength * 100);
        }
    }

    http.end();

    Serial.printf("Bytes written: %d/%d\n", totalWritten, contentLength);

    if (totalWritten != contentLength) {
        Serial.printf("Written only %d/%d bytes. Update failed.\n", totalWritten, contentLength);
        Serial.printf("Update error: %s\n", Update.errorString());
        return false;
    }
    
    if (!Update.end()) {
        Serial.printf("Error occurred from Update.end(): %s\n", Update.errorString());
        return false;
    }

    return true;
}
