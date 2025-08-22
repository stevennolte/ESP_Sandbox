/*
 * ESP32 IoT Device with OTA Updates
 * Features: LED control, Web interface, OTA updates
 */

#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPOTAUpdater.h>
#include <Update.h>
#include <FS.h>
#include <HTTPClient.h>
#include "Config.h"
#include "WiFiHelper.h"

// Get config instance
Config& config = Config::getInstance();

// --- Object Instances ---
Preferences preferences;
WebServer server(80);
WiFiClient espClient;
ESPOTAUpdater otaUpdater(ConfigConstants::Firmware::GITHUB_REPO, ConfigConstants::Firmware::VERSION);

// --- Function Declarations ---
float readCPUTemperature();
String getBoardType();
String loadHTMLTemplate(const char* filename);
void handleFileList();
void handleFileDownload();
void handleFileUpload();
void handleFileUploadComplete();
void handleFirmwareUpload();
void handleFirmwareUploadComplete();
void handleRoot();
void handleSetClientId();
void handleBrightness();
void handleReboot();
void setupWebServer();
void loadClientId();
void ensureTemplateExists();
void handleDebug();

// --- Utility Functions ---
String makeGitHubAPICall(const String& endpoint);
bool downloadFileFromGitHub(const String& filePath, const String& localPath);
void updateStoredCommitHash();
String loadTemplate(const char* templatePath);
bool downloadTemplate();
void checkForTemplateUpdate();
void forceTemplateUpdate();

// --- OTA Update Callbacks ---
void onUpdateComplete(bool success, const String& message) {
  if (success) {
    Serial.println("*** OTA UPDATE SUCCESSFUL ***");
    
    // Download latest templates after successful firmware update
    Serial.println("Downloading latest web templates...");
    if (downloadTemplate()) {
      updateStoredCommitHash();
      Serial.println("✓ Templates updated with firmware");
    } else {
      Serial.println("⚠ Some templates failed to download - device will attempt to download missing templates on next boot");
    }
    
    Serial.println("Rebooting...");
  } else {
    Serial.println("*** OTA UPDATE FAILED ***");
    Serial.printf("Error: %s\n", message.c_str());
  }
}

// --- Board Type Detection ---
String getBoardType() {
#ifdef BOARD_TYPE
  return String(BOARD_TYPE);
#else
  return "Unknown";
#endif
}

// --- Temperature Functions ---
float readCPUTemperature() {
  // ESP32 internal temperature sensor
  // Note: This is not very accurate and is mainly for monitoring purposes
  return temperatureRead();
}

// Function to load and process HTML template
String loadHTMLTemplate(const char* filename) {
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
  html.replace("{{CLIENT_ID}}", config.client_id);
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{LED_BRIGHTNESS}}", String(config.led_brightness));
  html.replace("{{WIFI_RSSI}}", String(WiFi.RSSI()));
  html.replace("{{WIFI_STATUS}}", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  
  // Add template version info
  preferences.begin("esp-config", true);
  String templateCommit = preferences.getString("last_commit", "Unknown");
  preferences.end();
  html.replace("{{TEMPLATE_VERSION}}", templateCommit.length() > 7 ? templateCommit.substring(0, 7) : templateCommit);
  
  return html;
}

// --- Web Server Functions ---
void handleRoot() {
  String html = loadHTMLTemplate("/index.html");
  server.send(200, "text/html", html);
}

void handleSetClientId() {
  if (server.hasArg("client_id")) {
    String newClientId = server.arg("client_id");
    if (newClientId.length() > 0 && newClientId.length() <= 32) {
      config.saveClientId(newClientId);
      
      // Restart mDNS with new hostname
      MDNS.end();
      if (!MDNS.begin(config.getClientId())) {
        Serial.println("Error restarting mDNS with new hostname");
      } else {
        Serial.println("mDNS restarted with new hostname: " + config.client_id);
        MDNS.addService("http", "tcp", 80);
      }
      
      String html = loadTemplate("simple_response.html");
      html.replace("{{TITLE}}", "Updated");
      html.replace("{{HEADER}}", "Client ID Updated");
      html.replace("{{MESSAGE}}", "New Client ID: <strong>" + config.client_id + "</strong>");
      html.replace("{{EXTRA_CONTENT}}", 
        "<p>New mDNS address: <strong>http://" + config.client_id + ".local</strong></p>");
      server.send(200, "text/html", html);
    } else {
      server.send(400, "text/plain", "Invalid client ID. Must be 1-32 characters.");
    }
  } else {
    server.send(400, "text/plain", "Missing client_id parameter");
  }
}

void handleBrightness() {
  if (server.hasArg("brightness")) {
    int newBrightness = server.arg("brightness").toInt();
    if (newBrightness >= 0 && newBrightness <= 255) {
      config.saveLedBrightness(newBrightness);
      
      String html = loadTemplate("simple_response.html");
      html.replace("{{TITLE}}", "Brightness Updated");
      html.replace("{{HEADER}}", "LED Brightness Updated");
      html.replace("{{MESSAGE}}", "New Brightness: <strong>" + String(config.led_brightness) + "</strong>");
      html.replace("{{EXTRA_CONTENT}}", "");
      server.send(200, "text/html", html);
    } else {
      server.send(400, "text/plain", "Invalid brightness value. Must be 0-255.");
    }
  } else {
    server.send(400, "text/plain", "Missing brightness parameter");
  }
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting device...");
  delay(1000);
  ESP.restart();
}

// --- File Management Functions ---
void handleFileList() {
  String html = loadTemplate("file_manager.html");
  
  // Build file list
  String fileList = "";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    fileList += "<tr><td>" + fileName + "</td>";
    fileList += "<td>" + String(file.size()) + " bytes</td>";
    fileList += "<td><a href='/download?file=" + fileName + "'>Download</a></td></tr>";
    file = root.openNextFile();
  }
  
  // Replace placeholder
  html.replace("{{FILE_LIST}}", fileList);
  
  server.send(200, "text/html", html);
}

void handleFileDownload() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file parameter");
    return;
  }
  
  String filename = server.arg("file");
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }
  
  if (!LittleFS.exists(filename)) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  
  File file = LittleFS.open(filename, "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file");
    return;
  }
  
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    Serial.printf("Upload Start: %s\n", filename.c_str());
    
    File file = LittleFS.open(filename, "w");
    if (!file) {
      Serial.println("Failed to create file");
      return;
    }
    file.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = LittleFS.open("/" + upload.filename, "a");
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("Upload End: %s, Size: %u\n", upload.filename.c_str(), upload.totalSize);
  }
}

void handleFileUploadComplete() {
  String html = loadTemplate("upload_complete.html");
  server.send(200, "text/html", html);
}

// --- Firmware Upload Functions ---
void handleFirmwareUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Firmware Upload Start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Firmware Update Success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleFirmwareUploadComplete() {
  String html = loadTemplate("firmware_complete.html");
  String content = "";
  
  if (Update.hasError()) {
    content += "<div class='status-icon error'>❌</div>";
    content += "<h1 class='error'>Firmware Update Failed</h1>";
    content += "<p>Error: " + String(Update.getError()) + "</p>";
    content += "<p><a href='/'>← Back to Main</a></p>";
  } else {
    content += "<div class='status-icon success'>✅</div>";
    content += "<h1 class='success'>Firmware Update Successful</h1>";
    content += "<p>Device will reboot in 3 seconds...</p>";
    content += "<script>setTimeout(function(){window.location.href='/';}, 5000);</script>";
  }
  
  html.replace("{{FIRMWARE_CONTENT}}", content);
  server.send(200, "text/html", html);
  
  if (!Update.hasError()) {
    delay(ConfigConstants::Timing::REBOOT_DELAY);
    ESP.restart();
  }
}

// --- Debug Page ---
void handleDebug() {
  String html = loadTemplate("debug.html");
  
  // Build debug sections
  String debugSections = "";
  
  // System Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>💻 System Information</h2>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Board Type:</span><span class='debug-value'>" + getBoardType() + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Firmware Version:</span><span class='debug-value'>" + String(ConfigConstants::Firmware::VERSION) + " (v" + String(ConfigConstants::Firmware::VERSION/100) + "." + String(ConfigConstants::Firmware::VERSION%100) + ")</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Chip Model:</span><span class='debug-value'>" + String(ESP.getChipModel()) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Chip Cores:</span><span class='debug-value'>" + String(ESP.getChipCores()) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>CPU Frequency:</span><span class='debug-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Flash Size:</span><span class='debug-value'>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Free Heap:</span><span class='debug-value'>" + String(ESP.getFreeHeap()) + " bytes</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Min Free Heap:</span><span class='debug-value'>" + String(ESP.getMinFreeHeap()) + " bytes</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Max Alloc Heap:</span><span class='debug-value'>" + String(ESP.getMaxAllocHeap()) + " bytes</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Uptime:</span><span class='debug-value'>" + String(millis() / 1000) + " seconds</span></div>";
  debugSections += "</div>";
  
  // Network Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>📡 Network Information</h2>";
  debugSections += WiFiHelper::getDebugInfo();
  debugSections += "</div>";
  
  // Sensor Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>🌡️ Sensor Information</h2>";
  float cpuTemp = readCPUTemperature();
  debugSections += "<div class='debug-item'><span class='debug-label'>CPU Temperature:</span><span class='debug-value'>" + String(cpuTemp, 1) + "°C</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>LED Brightness:</span><span class='debug-value'>" + String(config.led_brightness) + "/255</span></div>";
  debugSections += "</div>";
  
  // Timing Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>⏰ Timing Information</h2>";
  unsigned long currentTime = millis();
  debugSections += "<div class='debug-item'><span class='debug-label'>Current Time:</span><span class='debug-value'>" + String(currentTime) + " ms</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Last Update Check:</span><span class='debug-value'>" + String(config.last_update_check) + " ms</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Time Since Update Check:</span><span class='debug-value'>" + String((currentTime - config.last_update_check) / 1000) + " seconds</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Last WiFi Check:</span><span class='debug-value'>" + String(config.last_wifi_check) + " ms</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Time Since WiFi Check:</span><span class='debug-value'>" + String((currentTime - config.last_wifi_check) / 1000) + " seconds</span></div>";
  debugSections += "</div>";
  
  // Storage Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>💾 Storage Information</h2>";
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  debugSections += "<div class='debug-item'><span class='debug-label'>LittleFS Total:</span><span class='debug-value'>" + String(totalBytes) + " bytes (" + String(totalBytes/1024) + " KB)</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>LittleFS Used:</span><span class='debug-value'>" + String(usedBytes) + " bytes (" + String(usedBytes/1024) + " KB)</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>LittleFS Free:</span><span class='debug-value'>" + String(totalBytes - usedBytes) + " bytes (" + String((totalBytes - usedBytes)/1024) + " KB)</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Usage Percentage:</span><span class='debug-value'>" + String((usedBytes * 100) / totalBytes) + "%</span></div>";
  debugSections += "</div>";
  
  // Configuration Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>⚙️ Configuration</h2>";
  preferences.begin("esp-config", true);
  String storedCommit = preferences.getString("last_commit", "Unknown");
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  String storedClientId = preferences.getString("client_id", "Not Set");
  int storedBrightness = preferences.getInt("led_brightness", 0);
  String storedSSID = preferences.getString("wifi_ssid", "Not Set");
  preferences.end();
  
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored Client ID:</span><span class='debug-value'>" + storedClientId + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored LED Brightness:</span><span class='debug-value'>" + String(storedBrightness) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored WiFi SSID:</span><span class='debug-value'>" + storedSSID + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored Template Commit:</span><span class='debug-value'>" + (storedCommit.length() > 7 ? storedCommit.substring(0, 7) : storedCommit) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored Firmware Version:</span><span class='debug-value'>" + String(storedFirmwareVersion) + " (v" + String(storedFirmwareVersion/100) + "." + String(storedFirmwareVersion%100) + ")</span></div>";
  debugSections += "</div>";
  
  // Replace placeholder
  html.replace("{{DEBUG_SECTIONS}}", debugSections);
  
  server.send(200, "text/html", html);
}

// --- Utility Functions ---
String makeGitHubAPICall(const String& endpoint) {
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

bool downloadFileFromGitHub(const String& filePath, const String& localPath) {
  HTTPClient http;
  String url = "https://raw.githubusercontent.com/" + String(ConfigConstants::Firmware::GITHUB_REPO) + "/main/" + filePath;
  
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

void updateStoredCommitHash() {
  String response = makeGitHubAPICall("commits/main");
  if (response.length() > 0) {
    JsonDocument doc;
    doc.shrinkToFit();
    if (deserializeJson(doc, response) == DeserializationError::Ok) {
      String latestCommit = doc["sha"].as<String>();
      preferences.begin("esp-config", false);
      preferences.putString("last_commit", latestCommit);
      preferences.end();
      Serial.printf("Updated commit hash: %s\n", latestCommit.c_str());
    }
  }
}

// --- Template Loading Function ---
String loadTemplate(const char* templatePath) {
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

// --- Template Update Functions ---
void handleUpdateTemplate() {
  String html = loadTemplate("template_update.html");
  
  // Show current template info
  preferences.begin("esp-config", true);
  String currentCommit = preferences.getString("last_commit", "Unknown");
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  preferences.end();
  
  // Replace placeholders
  html.replace("{{GITHUB_REPO}}", String(ConfigConstants::Firmware::GITHUB_REPO));
  html.replace("{{CURRENT_COMMIT}}", currentCommit.length() > 7 ? currentCommit.substring(0, 7) : currentCommit);
  html.replace("{{TEMPLATE_FIRMWARE_VERSION}}", "v" + String(storedFirmwareVersion/100) + "." + String(storedFirmwareVersion%100));
  html.replace("{{CURRENT_FIRMWARE_VERSION}}", "v" + String(ConfigConstants::Firmware::VERSION/100) + "." + String(ConfigConstants::Firmware::VERSION%100));
  
  server.send(200, "text/html", html);
}

void handleUpdateTemplateAction() {
  Serial.println("Manual template update requested...");
  checkForTemplateUpdate();
  server.send(200, "text/plain", "Template check completed - see serial output for details");
}

void handleForceTemplateUpdate() {
  Serial.println("Force template update requested...");
  forceTemplateUpdate();
  server.send(200, "text/plain", "Force template update completed - see serial output for details");
}

bool downloadTemplate() {
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

void checkForTemplateUpdate() {
  Serial.println("Checking for template updates...");
  
  String response = makeGitHubAPICall("commits/main");
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
  
  preferences.begin("esp-config", true);
  String storedCommit = preferences.getString("last_commit", "");
  preferences.end();
  Serial.printf("Stored commit: %s\n", storedCommit.c_str());
  
  if (storedCommit != latestCommit) {
    Serial.println("Template update needed, downloading all template files...");
    if (downloadTemplate()) {
      preferences.begin("esp-config", false);
      preferences.putString("last_commit", latestCommit);
      preferences.end();
      Serial.println("✓ All templates updated successfully");
    } else {
      Serial.println("⚠ Some templates failed to download");
    }
  } else {
    Serial.println("Templates are up to date");
  }
}

void forceTemplateUpdate() {
  Serial.println("Force updating all templates...");
  
  if (downloadTemplate()) {
    updateStoredCommitHash();
    Serial.println("✓ Force update of all templates complete");
  } else {
    Serial.println("⚠ Force update completed with some failures");
  }
}

void ensureTemplateExists() {
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
  preferences.begin("esp-config", true);
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  preferences.end();
  
  if (storedFirmwareVersion != ConfigConstants::Firmware::VERSION) {
    Serial.printf("Firmware updated from v%d.%d to v%d.%d, downloading latest templates...\n", 
                  storedFirmwareVersion/100, storedFirmwareVersion%100,
                  ConfigConstants::Firmware::VERSION/100, ConfigConstants::Firmware::VERSION%100);
    
    // Download latest templates
    forceTemplateUpdate();
    
    // Update stored firmware version
    preferences.begin("esp-config", false);
    preferences.putInt("last_firmware_version", ConfigConstants::Firmware::VERSION);
    preferences.end();
    
    Serial.println("✓ Templates synchronized with new firmware");
  } else {
    Serial.println("✓ Templates exist and firmware version matches");
  }
}

// --- Web Server Setup ---
void setupWebServer() {
  // Initialize mDNS
  if (!MDNS.begin(config.getClientId())) {
    Serial.println("ERROR: mDNS failed to start");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("✓ mDNS: http://%s.local\n", config.getClientId());
  }
  
  // Setup routes
  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handleSetClientId);
  server.on("/brightness", HTTP_POST, handleBrightness);
  server.on("/reboot", handleReboot);
  
  // File management routes
  server.on("/files", handleFileList);
  server.on("/download", handleFileDownload);
  server.on("/upload", HTTP_POST, handleFileUploadComplete, handleFileUpload);
  
  // Firmware upload routes
  server.on("/firmware", []() {
    String html = loadTemplate("firmware_upload.html");
    server.send(200, "text/html", html);
  });
  server.on("/firmware-upload", HTTP_POST, handleFirmwareUploadComplete, handleFirmwareUpload);
  
  // WiFi configuration routes
  server.on("/wifi", []() { WiFiHelper::handleConfig(server); });
  server.on("/wifi-update", HTTP_POST, []() { WiFiHelper::handleUpdate(server); });
  server.on("/scan-networks", []() { WiFiHelper::handleNetworkScan(server); });
  
  // Template update routes
  server.on("/update-template", handleUpdateTemplate);
  server.on("/update-template-action", HTTP_POST, handleUpdateTemplateAction);
  server.on("/force-template-update", HTTP_POST, handleForceTemplateUpdate);
  
  // Debug page route
  server.on("/debug", handleDebug);
  
  server.begin();
  Serial.printf("✓ Web server: http://%s\n", WiFi.localIP().toString().c_str());
}

// --- Configuration Management ---
void loadClientId() {
  config.loadFromPreferences();
}

void setup() {
  // Initialize configuration
  config.begin();
  
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\n=== ESP32 IoT Device Starting ===");
  Serial.printf("Board Type: %s\n", getBoardType().c_str());
  Serial.printf("Firmware Version: %d (v%d.%d)\n", ConfigConstants::Firmware::VERSION, ConfigConstants::Firmware::VERSION/100, ConfigConstants::Firmware::VERSION%100);
  
  // Initialize hardware
  ledcSetup(ConfigConstants::Hardware::LED_CHANNEL, ConfigConstants::Hardware::LED_FREQ, ConfigConstants::Hardware::LED_RESOLUTION);
  ledcAttachPin(ConfigConstants::Hardware::LED_PIN, ConfigConstants::Hardware::LED_CHANNEL);
  ledcWrite(ConfigConstants::Hardware::LED_CHANNEL, 0); // Start with LED off
  
  // Initialize filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: Failed to mount LittleFS");
    return;
  }
  Serial.println("✓ LittleFS mounted");

  // Load saved configuration
  loadClientId();

  // Connect to WiFi
  WiFiHelper::setup();
  if (!WiFiHelper::isConnected()) {
    Serial.println("ERROR: Cannot continue without WiFi");
    return;
  }
  
  // Ensure web template exists and is up to date
  ensureTemplateExists();

  // Debug: List all files in LittleFS
  Serial.println("=== LittleFS Contents ===");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("DIR:  %s\n", file.name());
      File subFile = file.openNextFile();
      while (subFile) {
        Serial.printf("  FILE: %s (%d bytes)\n", subFile.name(), subFile.size());
        subFile = file.openNextFile();
      }
    } else {
      Serial.printf("FILE: %s (%d bytes)\n", file.name(), file.size());
    }
    file = root.openNextFile();
  }
  root.close();
  Serial.println("=== End LittleFS Contents ===");

  // Initialize web server
  setupWebServer();

  // Initialize OTA updater
  otaUpdater.setUpdateCompleteCallback(onUpdateComplete);
  otaUpdater.setBoardType(getBoardType());
  otaUpdater.enableAutoUpdate(true); // Let the library handle the update process

  // Perform initial update check
  Serial.println("Checking for firmware updates...");
  otaUpdater.checkForUpdates();
  config.updateUpdateCheckTime(millis());
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  unsigned long currentTime = millis();

  // LED heartbeat indicator
  ledcWrite(ConfigConstants::Hardware::LED_CHANNEL, config.led_brightness);
  delay(ConfigConstants::Timing::LED_PULSE_DURATION);
  ledcWrite(ConfigConstants::Hardware::LED_CHANNEL, 0);
  
  // Handle web server requests
  server.handleClient();

  // Periodic tasks with timing
  
  // WiFi connection monitoring (every 30 seconds)
  if (config.shouldCheckWiFi(currentTime)) {
    WiFiHelper::checkConnection();
    config.updateWiFiCheckTime(currentTime);
  }
  
  // OTA update checking (every 5 minutes)
  if (config.shouldCheckUpdates(currentTime)) {
    otaUpdater.checkForUpdates();
    config.updateUpdateCheckTime(currentTime);
  }

  delay(ConfigConstants::Timing::MAIN_LOOP_DELAY);
}

