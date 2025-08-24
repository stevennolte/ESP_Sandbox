#include "TemplateManager.h"
#include "WiFi.h"

Config& TemplateManager::_config = Config::getInstance();
Preferences TemplateManager::_preferences;

String TemplateManager::loadHTMLTemplate(const char* filename) {
    if (!LittleFS.exists(filename)) {
        return "<!DOCTYPE html><html><body><h1>Error: Template file not found</h1></body></html>";
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return "<!DOCTYPE html><html><body><h1>Error: Could not open template file</h1></body></html>";
    }
    
    String html = file.readString();
    file.close();
    
    // Replace placeholders with actual values
    html.replace("{{CLIENT_ID}}", _config.client_id);
    html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
    html.replace("{{LED_BRIGHTNESS}}", String(_config.led_brightness));
    html.replace("{{WIFI_RSSI}}", String(WiFi.RSSI()));
    html.replace("{{WIFI_STATUS}}", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    
    // Add template version info
    _preferences.begin("esp-config", true);
    String templateCommit = _preferences.getString("last_commit", "Unknown");
    _preferences.end();
    html.replace("{{TEMPLATE_VERSION}}", templateCommit.length() > 7 ? templateCommit.substring(0, 7) : templateCommit);
    
    return html;
}

String TemplateManager::loadTemplate(const char* templatePath) {
    String fullPath = "/templates/" + String(templatePath);
    
    Serial.printf("Attempting to load template: %s\n", fullPath.c_str());
    
    if (!LittleFS.exists(fullPath)) {
        Serial.printf("Template not found: %s\n", fullPath.c_str());
        
        // List all files in /templates directory for debugging
        if (LittleFS.exists("/templates")) {
            Serial.println("Contents of /templates directory:");
            File templatesDir = LittleFS.open("/templates");
            File file = templatesDir.openNextFile();
            while (file) {
                Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
                file = templatesDir.openNextFile();
            }
            templatesDir.close();
        } else {
            Serial.println("/templates directory does not exist");
        }
        
        return "<!DOCTYPE html><html><body><h1>Error: Template not found</h1><p>Path: " + fullPath + "</p></body></html>";
    }
    
    File file = LittleFS.open(fullPath, "r");
    if (!file) {
        Serial.printf("Failed to open template: %s\n", fullPath.c_str());
        return "<!DOCTYPE html><html><body><h1>Error: Could not open template</h1><p>Path: " + fullPath + "</p></body></html>";
    }
    
    String html = file.readString();
    file.close();
    Serial.printf("✓ Template loaded successfully: %s (%d bytes)\n", fullPath.c_str(), html.length());
    return html;
}

String TemplateManager::makeGitHubAPICall(const String& endpoint) {
    HTTPClient http;
    String url = "https://api.github.com/repos/" + String(ConfigConstants::Firmware::GITHUB_REPO) + "/" + endpoint;
    
    http.begin(url);
    http.addHeader("User-Agent", ConfigConstants::Network::USER_AGENT_CHECKER);
    http.setTimeout(ConfigConstants::Network::HTTP_TIMEOUT_SHORT);
    
    int httpCode = http.GET();
    String result = "";
    
    if (httpCode == HTTP_CODE_OK) {
        result = http.getString();
    } else {
        Serial.printf("GitHub API call failed: HTTP %d\n", httpCode);
    }
    
    http.end();
    return result;
}

bool TemplateManager::downloadFileFromGitHub(const String& filePath, const String& localPath) {
    HTTPClient http;
    String url = "https://raw.githubusercontent.com/" + String(ConfigConstants::Firmware::GITHUB_REPO) + "/" + String(ConfigConstants::Firmware::GITHUB_BRANCH) + "/" + filePath;
    
    http.begin(url);
    http.addHeader("User-Agent", ConfigConstants::Network::USER_AGENT_TEMPLATE);
    http.setTimeout(ConfigConstants::Network::HTTP_TIMEOUT_LONG);
    
    int httpCode = http.GET();
    bool success = false;
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        if (payload.length() > 0) {
            File file = LittleFS.open(localPath, "w");
            if (file) {
                size_t written = file.print(payload);
                file.close();
                success = written > 0;
                Serial.printf("Downloaded %d bytes to %s\n", written, localPath.c_str());
            }
        }
    } else {
        Serial.printf("Download failed: HTTP %d\n", httpCode);
    }
    
    http.end();
    return success;
}

void TemplateManager::updateStoredCommitHash() {
    String response = makeGitHubAPICall("commits/" + String(ConfigConstants::Firmware::GITHUB_BRANCH));
    if (response.length() > 0) {
        JsonDocument doc;
        doc.shrinkToFit();
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            String latestCommit = doc["sha"].as<String>();
            _preferences.begin("esp-config", false);
            _preferences.putString("last_commit", latestCommit);
            _preferences.end();
            Serial.printf("Updated commit hash: %s\n", latestCommit.c_str());
        }
    }
}

bool TemplateManager::downloadTemplate() {
    Serial.println("Starting template download...");
    
    // Create templates directory if it doesn't exist
    if (!LittleFS.exists("/templates")) {
        Serial.println("Creating /templates directory...");
        LittleFS.mkdir("/templates");
        if (LittleFS.exists("/templates")) {
            Serial.println("✓ /templates directory created");
        } else {
            Serial.println("✗ Failed to create /templates directory");
        }
    }
    
    // List of all template files to download
    const char* templateFiles[] = {
        "index.html",
        "templates/file_manager.html",
        "templates/wifi_config.html", 
        "templates/debug.html",
        "templates/template_update.html",
        "templates/firmware_upload.html",
        "templates/simple_response.html",
        "templates/upload_complete.html",
        "templates/firmware_complete.html",
        "templates/wifi_updated.html"
    };
    
    const char* localPaths[] = {
        "/index.html",
        "/templates/file_manager.html",
        "/templates/wifi_config.html",
        "/templates/debug.html", 
        "/templates/template_update.html",
        "/templates/firmware_upload.html",
        "/templates/simple_response.html",
        "/templates/upload_complete.html",
        "/templates/firmware_complete.html",
        "/templates/wifi_updated.html"
    };
    
    bool allSuccess = true;
    int fileCount = sizeof(templateFiles) / sizeof(templateFiles[0]);
    
    Serial.printf("Downloading %d template files...\n", fileCount);
    
    for (int i = 0; i < fileCount; i++) {
        Serial.printf("Downloading %s...\n", templateFiles[i]);
        
        if (downloadFileFromGitHub(String("data/") + templateFiles[i], localPaths[i])) {
            Serial.printf("✓ Downloaded %s\n", templateFiles[i]);
        } else {
            Serial.printf("✗ Failed to download %s\n", templateFiles[i]);
            allSuccess = false;
        }
        
        // Small delay between downloads to avoid overwhelming the server
        delay(100);
    }
    
    if (allSuccess) {
        Serial.println("✓ All template files downloaded successfully");
    } else {
        Serial.println("⚠ Some template files failed to download");
    }
    
    return allSuccess;
}

void TemplateManager::checkForTemplateUpdate() {
    Serial.println("Checking for template updates...");
    
    String response = makeGitHubAPICall("commits/" + String(ConfigConstants::Firmware::GITHUB_BRANCH));
    if (response.length() == 0) {
        Serial.println("Failed to get GitHub API response");
        return;
    }

    JsonDocument doc;
    doc.shrinkToFit();
    if (deserializeJson(doc, response) != DeserializationError::Ok) {
        Serial.println("Failed to parse GitHub API response");
        return;
    }

    String latestCommit = doc["sha"].as<String>();
    Serial.printf("Latest commit: %s\n", latestCommit.c_str());
    
    _preferences.begin("esp-config", true);
    String storedCommit = _preferences.getString("last_commit", "");
    _preferences.end();
    Serial.printf("Stored commit: %s\n", storedCommit.c_str());
    
    if (storedCommit != latestCommit) {
        Serial.println("Template update needed, downloading all template files...");
        if (downloadTemplate()) {
            _preferences.begin("esp-config", false);
            _preferences.putString("last_commit", latestCommit);
            _preferences.end();
            Serial.println("✓ All templates updated successfully");
        } else {
            Serial.println("⚠ Some templates failed to download");
        }
    } else {
        Serial.println("Templates are up to date");
    }
}

void TemplateManager::forceTemplateUpdate() {
    Serial.println("Force updating all templates...");
    
    if (downloadTemplate()) {
        updateStoredCommitHash();
        Serial.println("✓ Force update of all templates complete");
    } else {
        Serial.println("⚠ Force update completed with some failures");
    }
}

void TemplateManager::ensureTemplateExists() {
    // Check if main template files exist
    bool templatesExist = LittleFS.exists("/index.html") && 
                         LittleFS.exists("/templates") &&
                         LittleFS.exists("/templates/debug.html") &&
                         LittleFS.exists("/templates/file_manager.html");
    
    if (!templatesExist) {
        Serial.println("Template files not found, downloading from GitHub...");
        forceTemplateUpdate();
        return;
    }
    
    // Check if this is the first boot after a firmware update
    _preferences.begin("esp-config", true);
    int storedFirmwareVersion = _preferences.getInt("last_firmware_version", 0);
    _preferences.end();
    
    if (storedFirmwareVersion != ConfigConstants::Firmware::VERSION) {
        Serial.printf("Firmware updated from v%d.%d to v%d.%d, downloading latest templates...\n", 
                      storedFirmwareVersion/100, storedFirmwareVersion%100,
                      ConfigConstants::Firmware::VERSION/100, ConfigConstants::Firmware::VERSION%100);
        
        // Download latest templates
        forceTemplateUpdate();
        
        // Update stored firmware version
        _preferences.begin("esp-config", false);
        _preferences.putInt("last_firmware_version", ConfigConstants::Firmware::VERSION);
        _preferences.end();
        
        Serial.println("✓ Templates synchronized with new firmware");
    } else {
        Serial.println("✓ Templates exist and firmware version matches");
    }
}
