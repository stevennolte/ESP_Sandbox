/*
 * ESP32 IoT Device with OTA Updates and Watchdog Timer
 * Features: LED control, Web interface, OTA updates, Recovery mode
 */

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPOTAUpdater.h>
#include <Update.h>
#include <FS.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include "Config.h"
#include "WiFiHelper.h"
#include "SystemUtils.h"
#include <StatusIndicator.h>

// Get config instance
Config& config = Config::getInstance();

// --- Status Indicator ---
StatusIndicator statusLED;

// --- Watchdog Variables ---
bool recoveryMode = false;
unsigned long lastWatchdogFeed = 0;

// --- Object Instances ---
Preferences preferences;
WebServer server(80);
ESPOTAUpdater otaUpdater(ConfigConstants::Firmware::GITHUB_REPO, ConfigConstants::Firmware::VERSION);

// --- Function Declarations ---
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
void handleDebugData(); // New API endpoint for real-time data

// --- Watchdog Functions ---
void initWatchdog();
void feedWatchdog();
void checkWatchdogTimeout();
bool isRecoveryMode();
void enterRecoveryMode();

// --- Utility Functions ---
String makeGitHubAPICall(const String& endpoint);
bool downloadFileFromGitHub(const String& filePath, const String& localPath);
void updateStoredCommitHash();
String loadTemplate(const char* templatePath);
bool downloadTemplate();
void checkForTemplateUpdate();
void forceTemplateUpdate();

// --- Watchdog Functions ---
void initWatchdog() {
  Serial.println("Initializing software watchdog timer...");
  
  // Check if this boot was due to a software watchdog timeout
  preferences.begin("esp-config", true);
  bool watchdogTimeout = preferences.getBool("watchdog_timeout", false);
  preferences.end();
  
  // Check if this is a recovery boot
  esp_reset_reason_t resetReason = esp_reset_reason();
  if (resetReason == ESP_RST_TASK_WDT || resetReason == ESP_RST_WDT || resetReason == ESP_RST_PANIC || watchdogTimeout) {
    Serial.println("*** WATCHDOG/PANIC RESET DETECTED - ENTERING RECOVERY MODE ***");
    recoveryMode = true;
    
    // Store recovery mode flag in preferences
    preferences.begin("esp-config", false);
    preferences.putBool("recovery_mode", true);
    preferences.putULong("recovery_time", millis());
    preferences.putBool("watchdog_timeout", false); // Clear the flag
    preferences.end();
  } else {
    // Clear recovery mode flag on normal boot
    preferences.begin("esp-config", false);
    preferences.putBool("recovery_mode", false);
    preferences.putBool("watchdog_timeout", false);
    preferences.end();
  }
  
  // Initialize software watchdog (we'll use a simple timer-based approach)
  lastWatchdogFeed = millis();
  Serial.printf("✓ Software watchdog initialized: %d second timeout\n", ConfigConstants::Timing::WATCHDOG_TIMEOUT);
}

void feedWatchdog() {
  lastWatchdogFeed = millis();
}

void checkWatchdogTimeout() {
  unsigned long timeSinceLastFeed = millis() - lastWatchdogFeed;
  if (timeSinceLastFeed > (ConfigConstants::Timing::WATCHDOG_TIMEOUT * 1000)) {
    Serial.println("*** SOFTWARE WATCHDOG TIMEOUT - RESTARTING ***");
    
    // Mark as watchdog timeout for next boot
    preferences.begin("esp-config", false);
    preferences.putBool("watchdog_timeout", true);
    preferences.end();
    
    delay(1000);
    ESP.restart();
  }
}

bool isRecoveryMode() {
  return recoveryMode;
}

void enterRecoveryMode() {
  Serial.println("*** ENTERING RECOVERY MODE ***");
  recoveryMode = true;
  
  // Store recovery state
  preferences.begin("esp-config", false);
  preferences.putBool("recovery_mode", true);
  preferences.putULong("recovery_time", millis());
  preferences.end();
  
  // Simple recovery web server with minimal functionality
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>🔧 Recovery Mode - " + String(config.getClientId()) + "</title>";
    html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px;background:#f5f5f5}";
    html += ".container{background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += ".warning{background:#fff3cd;border:1px solid #ffeaa7;color:#856404;padding:15px;border-radius:5px;margin:20px 0}";
    html += ".button{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px}";
    html += ".button:hover{background:#0056b3}</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>🔧 Recovery Mode</h1>";
    html += "<div class='warning'>⚠ <strong>Device is in recovery mode</strong><br>";
    html += "This occurred due to a watchdog timer reset, indicating the device may have frozen.</div>";
    html += "<h3>Device Information:</h3>";
    html += "<p><strong>Device ID:</strong> " + String(config.getClientId()) + "</p>";
    html += "<p><strong>IP Address:</strong> " + WiFiHelper::getLocalIP() + "</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</p>";
    html += "<p><strong>Reset Reason:</strong> " + String(esp_reset_reason()) + "</p>";
    html += "<h3>Available Actions:</h3>";
    html += "<form action='/exit-recovery' method='post' style='margin:20px 0'>";
    html += "<button type='submit' class='button'>Exit Recovery Mode & Restart</button></form>";
    html += "<p><small>Only basic functions are available in recovery mode.<br>";
    html += "Exiting recovery mode will restart the device with full functionality.</small></p>";
    html += "</div></body></html>";
    server.send(200, "text/html", html);
  });
  
  server.on("/exit-recovery", HTTP_POST, []() {
    preferences.begin("esp-config", false);
    preferences.putBool("recovery_mode", false);
    preferences.end();
    server.send(200, "text/html", "<!DOCTYPE html><html><body><h1>Exiting Recovery Mode</h1><p>Rebooting...</p></body></html>");
    delay(2000);
    ESP.restart();
  });
  
  server.begin();
  Serial.println("✓ Recovery mode web server started");
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
  html.replace("{{LED_BRIGHTNESS}}", String(config.led_brightness));
  
  // Use WiFiHelper for WiFi-related template variables
  html = WiFiHelper::processWiFiTemplateVariables(html);
  
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
      
      // Update status LED brightness
      statusLED.setBrightness(newBrightness);
      
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
  debugSections += "<div class='debug-item'><span class='debug-label'>Board Type:</span><span class='debug-value'>" + SystemUtils::getBoardType() + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Firmware Version:</span><span class='debug-value'>" + String(ConfigConstants::Firmware::VERSION) + " (v" + String(ConfigConstants::Firmware::VERSION/100) + "." + String(ConfigConstants::Firmware::VERSION%100) + ")</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Chip Model:</span><span class='debug-value'>" + String(ESP.getChipModel()) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Chip Cores:</span><span class='debug-value'>" + String(ESP.getChipCores()) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>CPU Frequency:</span><span class='debug-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Flash Size:</span><span class='debug-value'>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Free Heap:</span><span class='debug-value' data-id='system-free-heap'>" + String(ESP.getFreeHeap()) + " bytes</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Min Free Heap:</span><span class='debug-value' data-id='system-min-free-heap'>" + String(ESP.getMinFreeHeap()) + " bytes</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Max Alloc Heap:</span><span class='debug-value' data-id='system-max-alloc-heap'>" + String(ESP.getMaxAllocHeap()) + " bytes</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Uptime:</span><span class='debug-value' data-id='system-uptime'>" + String(millis() / 1000) + " seconds</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Recovery Mode:</span><span class='debug-value'>" + String(recoveryMode ? "Yes" : "No") + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Last Watchdog Feed:</span><span class='debug-value' data-id='watchdog-last-feed'>" + String((millis() - lastWatchdogFeed) / 1000) + " seconds ago</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Reset Reason:</span><span class='debug-value'>" + String(esp_reset_reason()) + "</span></div>";
  debugSections += "</div>";
  
  // Network Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>📡 Network Information</h2>";
  debugSections += WiFiHelper::getDebugInfo();
  debugSections += "</div>";
  
  // Sensor Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>🌡️ Sensor Information</h2>";
  float cpuTemp = SystemUtils::readCPUTemperature();
  debugSections += "<div class='debug-item'><span class='debug-label'>CPU Temperature:</span><span class='debug-value' data-id='sensor-cpu-temp'>" + String(cpuTemp, 1) + "°C</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>LED Brightness:</span><span class='debug-value' data-id='sensor-led-brightness'>" + String(config.led_brightness) + "/255</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Status LED Type:</span><span class='debug-value'>" + String(statusLED.getLEDType() == LEDType::WS2812_LED ? "RGB LED" : "Single LED") + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Status LED Mode:</span><span class='debug-value' data-id='sensor-led-mode'>" + String((int)statusLED.getMode()) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Status LED Status:</span><span class='debug-value' data-id='sensor-led-status'>" + String((int)statusLED.getStatus()) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Status LED Enabled:</span><span class='debug-value' data-id='sensor-led-enabled'>" + String(statusLED.isEnabled() ? "Yes" : "No") + "</span></div>";
  debugSections += "</div>";
  
  // Timing Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>⏰ Timing Information</h2>";
  unsigned long currentTime = millis();
  debugSections += "<div class='debug-item'><span class='debug-label'>Current Time:</span><span class='debug-value' data-id='timing-current-time'>" + String(currentTime) + " ms</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Last Update Check:</span><span class='debug-value' data-id='timing-last-update-check'>" + String(config.last_update_check) + " ms</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Time Since Update Check:</span><span class='debug-value' data-id='timing-time-since-update-check'>" + String((currentTime - config.last_update_check) / 1000) + " seconds</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Last WiFi Check:</span><span class='debug-value' data-id='timing-last-wifi-check'>" + String(config.last_wifi_check) + " ms</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Time Since WiFi Check:</span><span class='debug-value' data-id='timing-time-since-wifi-check'>" + String((currentTime - config.last_wifi_check) / 1000) + " seconds</span></div>";
  debugSections += "</div>";
  
  // Storage Information
  debugSections += "<div class='debug-section'>";
  debugSections += "<h2>💾 Storage Information</h2>";
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  debugSections += "<div class='debug-item'><span class='debug-label'>LittleFS Total:</span><span class='debug-value'>" + String(totalBytes) + " bytes (" + String(totalBytes/1024) + " KB)</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>LittleFS Used:</span><span class='debug-value' data-id='storage-used'>" + String(usedBytes) + " bytes (" + String(usedBytes/1024) + " KB)</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>LittleFS Free:</span><span class='debug-value' data-id='storage-free'>" + String(totalBytes - usedBytes) + " bytes (" + String((totalBytes - usedBytes)/1024) + " KB)</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Usage Percentage:</span><span class='debug-value' data-id='storage-usage-percent'>" + String((usedBytes * 100) / totalBytes) + "%</span></div>";
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
  bool storedApMode = preferences.getBool("force_ap_mode", false);
  preferences.end();
  
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored Client ID:</span><span class='debug-value'>" + storedClientId + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored LED Brightness:</span><span class='debug-value'>" + String(storedBrightness) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored WiFi SSID:</span><span class='debug-value'>" + storedSSID + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>WiFi Boot Mode:</span><span class='debug-value'>" + String(storedApMode ? "Access Point" : "Station") + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored Template Commit:</span><span class='debug-value'>" + (storedCommit.length() > 7 ? storedCommit.substring(0, 7) : storedCommit) + "</span></div>";
  debugSections += "<div class='debug-item'><span class='debug-label'>Stored Firmware Version:</span><span class='debug-value'>" + String(storedFirmwareVersion) + " (v" + String(storedFirmwareVersion/100) + "." + String(storedFirmwareVersion%100) + ")</span></div>";
  debugSections += "</div>";
  
  // Replace placeholder
  html.replace("{{DEBUG_SECTIONS}}", debugSections);
  
  server.send(200, "text/html", html);
}

// --- Debug Data API (for real-time updates) ---
void handleDebugData() {
  JsonDocument doc;
  
  // System Information
  doc["system"]["boardType"] = SystemUtils::getBoardType();
  doc["system"]["firmwareVersion"] = ConfigConstants::Firmware::VERSION;
  doc["system"]["firmwareVersionText"] = "v" + String(ConfigConstants::Firmware::VERSION/100) + "." + String(ConfigConstants::Firmware::VERSION%100);
  doc["system"]["chipModel"] = ESP.getChipModel();
  doc["system"]["chipCores"] = ESP.getChipCores();
  doc["system"]["cpuFreq"] = ESP.getCpuFreqMHz();
  doc["system"]["flashSize"] = ESP.getFlashChipSize() / 1024 / 1024;
  doc["system"]["freeHeap"] = ESP.getFreeHeap();
  doc["system"]["minFreeHeap"] = ESP.getMinFreeHeap();
  doc["system"]["maxAllocHeap"] = ESP.getMaxAllocHeap();
  doc["system"]["uptime"] = millis() / 1000;
  doc["system"]["recoveryMode"] = recoveryMode;
  doc["system"]["lastWatchdogFeed"] = (millis() - lastWatchdogFeed) / 1000;
  doc["system"]["resetReason"] = esp_reset_reason();
  
  // Network Information
  doc["network"]["wifiStatus"] = WiFiHelper::getWiFiStatus();
  doc["network"]["wifiConnected"] = WiFiHelper::isWiFiConnected();
  doc["network"]["ssid"] = WiFiHelper::getSSID();
  doc["network"]["ipAddress"] = WiFiHelper::getLocalIP();
  doc["network"]["gateway"] = WiFiHelper::getGatewayIP();
  doc["network"]["dns"] = WiFiHelper::getDNSIP();
  doc["network"]["macAddress"] = WiFiHelper::getMACAddress();
  doc["network"]["rssi"] = WiFiHelper::getRSSI();
  
  // Sensor Information
  doc["sensors"]["cpuTemp"] = SystemUtils::readCPUTemperature();
  doc["sensors"]["ledBrightness"] = config.led_brightness;
  doc["sensors"]["statusLedType"] = statusLED.getLEDType() == LEDType::WS2812_LED ? "RGB" : "Single";
  doc["sensors"]["statusLedMode"] = (int)statusLED.getMode();
  doc["sensors"]["statusLedStatus"] = (int)statusLED.getStatus();
  doc["sensors"]["statusLedEnabled"] = statusLED.isEnabled();
  
  // Timing Information
  unsigned long currentTime = millis();
  doc["timing"]["currentTime"] = currentTime;
  doc["timing"]["lastUpdateCheck"] = config.last_update_check;
  doc["timing"]["timeSinceUpdateCheck"] = (currentTime - config.last_update_check) / 1000;
  doc["timing"]["lastWifiCheck"] = config.last_wifi_check;
  doc["timing"]["timeSinceWifiCheck"] = (currentTime - config.last_wifi_check) / 1000;
  
  // Storage Information
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  doc["storage"]["totalBytes"] = totalBytes;
  doc["storage"]["usedBytes"] = usedBytes;
  doc["storage"]["freeBytes"] = totalBytes - usedBytes;
  doc["storage"]["totalKB"] = totalBytes / 1024;
  doc["storage"]["usedKB"] = usedBytes / 1024;
  doc["storage"]["freeKB"] = (totalBytes - usedBytes) / 1024;
  doc["storage"]["usagePercent"] = (usedBytes * 100) / totalBytes;
  
  // Configuration Information
  preferences.begin("esp-config", true);
  String storedCommit = preferences.getString("last_commit", "Unknown");
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  String storedClientId = preferences.getString("client_id", "Not Set");
  int storedBrightness = preferences.getInt("led_brightness", 0);
  String storedSSID = preferences.getString("wifi_ssid", "Not Set");
  preferences.end();
  
  doc["config"]["storedClientId"] = storedClientId;
  doc["config"]["storedLedBrightness"] = storedBrightness;
  doc["config"]["storedWifiSSID"] = storedSSID;
  doc["config"]["storedTemplateCommit"] = storedCommit.length() > 7 ? storedCommit.substring(0, 7) : storedCommit;
  doc["config"]["storedFirmwareVersion"] = storedFirmwareVersion;
  doc["config"]["storedFirmwareVersionText"] = "v" + String(storedFirmwareVersion/100) + "." + String(storedFirmwareVersion%100);
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
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

void updateStoredCommitHash() {
  String response = makeGitHubAPICall("commits/" + String(ConfigConstants::Firmware::GITHUB_BRANCH));
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
  server.on("/wifi-mode", HTTP_POST, []() { WiFiHelper::handleWiFiModeToggle(server); });
  
  // Template update routes
  server.on("/update-template", []() {
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
  });
  server.on("/update-template-action", HTTP_POST, []() {
    Serial.println("Manual template update requested...");
    checkForTemplateUpdate();
    server.send(200, "text/plain", "Template check completed - see serial output for details");
  });
  server.on("/force-template-update", HTTP_POST, []() {
    Serial.println("Force template update requested...");
    forceTemplateUpdate();
    server.send(200, "text/plain", "Force template update completed - see serial output for details");
  });
  
  // Debug page route
  server.on("/debug", handleDebug);
  server.on("/debug-data", handleDebugData); // Real-time debug data API
  
  // Watchdog test endpoint (for testing only)
  server.on("/test-watchdog", []() {
    server.send(200, "text/plain", "Triggering watchdog timeout in 10 seconds...");
    Serial.println("*** WATCHDOG TEST: Stopping watchdog feeds ***");
    delay(35000); // This will trigger the watchdog timeout
  });
  
  // Status LED test endpoints
  server.on("/test-led", []() {
    String html = "<!DOCTYPE html><html><head><title>LED Test</title>";
    html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px}";
    html += ".button{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px;text-decoration:none;display:inline-block}</style></head><body>";
    html += "<h1>Status LED Test</h1>";
    html += "<p>Current LED Type: <strong>" + String(statusLED.getLEDType() == LEDType::WS2812_LED ? "RGB LED" : "Single LED") + "</strong></p>";
    html += "<p>Current Status: <strong>" + statusLED.getStatusString() + "</strong></p>";
    html += "<div>";
    html += "<a href='/led-normal' class='button'>Normal</a>";
    html += "<a href='/led-warning' class='button'>Warning</a>";
    html += "<a href='/led-error' class='button'>Error</a>";
    html += "<a href='/led-info' class='button'>Info</a>";
    html += "<a href='/led-success' class='button'>Success</a>";
    html += "<a href='/led-connecting' class='button'>Connecting</a>";
    html += "<a href='/led-recovery' class='button'>Recovery</a>";
    if (statusLED.getLEDType() == LEDType::WS2812_LED) {
      html += "<a href='/led-cycle' class='button'>RGB Cycle</a>";
    }
    html += "<a href='/led-off' class='button'>Off</a>";
    html += "</div>";
    html += "<p><a href='/'>← Back to Main</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  server.on("/led-normal", []() { statusLED.showNormal(); server.send(200, "text/plain", "LED: Normal"); });
  server.on("/led-warning", []() { statusLED.showWarning(); server.send(200, "text/plain", "LED: Warning"); });
  server.on("/led-error", []() { statusLED.showError(); server.send(200, "text/plain", "LED: Error"); });
  server.on("/led-info", []() { statusLED.showInfo(); server.send(200, "text/plain", "LED: Info"); });
  server.on("/led-success", []() { statusLED.showSuccess(); server.send(200, "text/plain", "LED: Success"); });
  server.on("/led-connecting", []() { statusLED.showConnecting(); server.send(200, "text/plain", "LED: Connecting"); });
  server.on("/led-recovery", []() { statusLED.showRecovery(); server.send(200, "text/plain", "LED: Recovery"); });
  server.on("/led-cycle", []() { 
    if (statusLED.getLEDType() == LEDType::WS2812_LED) {
      statusLED.setStatusWithMode(StatusType::NORMAL, IndicatorMode::RGB_CYCLE);
      server.send(200, "text/plain", "LED: RGB Cycle");
    } else {
      server.send(400, "text/plain", "RGB Cycle not available on single LED");
    }
  });
  server.on("/led-off", []() { statusLED.off(); server.send(200, "text/plain", "LED: Off"); });
  
  server.begin();
  Serial.printf("✓ Web server: http://%s\n", WiFiHelper::getLocalIP().c_str());
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
  Serial.printf("Board Type: %s\n", SystemUtils::getBoardType().c_str());
  Serial.printf("Firmware Version: %d (v%d.%d)\n", ConfigConstants::Firmware::VERSION, ConfigConstants::Firmware::VERSION/100, ConfigConstants::Firmware::VERSION%100);
  
  // Initialize status indicator
  statusLED.begin(config.led_brightness);
  statusLED.showConnecting();
  
  // Initialize watchdog timer
  initWatchdog();
  
  // Check if we're in recovery mode
  if (isRecoveryMode()) {
    Serial.println("⚠ Device is in recovery mode - limited functionality");
    statusLED.showRecovery();
    
    // Setup WiFi for recovery mode using WiFiHelper
    WiFiHelper::setupRecoveryWiFi();
    statusLED.showWarning();  // Recovery mode status
    
    enterRecoveryMode();
    return; // Skip normal setup in recovery mode
  }
  
  // Initialize hardware (LED now handled by StatusIndicator)
  // No need for manual LED setup anymore
  
  // Initialize filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: Failed to mount LittleFS");
    statusLED.showError();
    return;
  }
  Serial.println("✓ LittleFS mounted");

  // Load saved configuration
  loadClientId();

  // Connect to WiFi or start Access Point
  WiFiHelper::setup();
  if (!WiFiHelper::isConnected()) {
    Serial.println("ERROR: Failed to establish network connection");
    statusLED.showError();
    return;
  }
  
  // Log connection status
  Serial.println(WiFiHelper::getConnectionInfo());
  statusLED.showSuccess();
  delay(1000);
  
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
  otaUpdater.setBoardType(SystemUtils::getBoardType());
  otaUpdater.enableAutoUpdate(true); // Let the library handle the update process

  // Perform initial update check
  Serial.println("Checking for firmware updates...");
  otaUpdater.checkForUpdates();
  config.updateUpdateCheckTime(millis());
  
  // Set normal operation status
  statusLED.showNormal();
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  unsigned long currentTime = millis();

  // Update status indicator (must be called for animations)
  statusLED.update();

  // Feed the watchdog timer at the start of each loop
  if (!recoveryMode) {
    feedWatchdog();
    
    // Check for watchdog timeout (separate check for safety)
    checkWatchdogTimeout();
  }

  // Handle recovery mode separately
  if (recoveryMode) {
    server.handleClient();
    delay(100); // Shorter delay in recovery mode
    return;
  }

  // Handle web server requests
  server.handleClient();

  // Periodic tasks with timing
  
  // WiFi connection monitoring (every 30 seconds)
  if (config.shouldCheckWiFi(currentTime)) {
    WiFiHelper::checkConnection();
    config.updateWiFiCheckTime(currentTime);
    
    // Update status based on WiFi connection
    if (!WiFiHelper::isConnected()) {
      statusLED.showError();
    } else {
      statusLED.showNormal();
    }
  }
  
  // OTA update checking (every 5 minutes)
  if (config.shouldCheckUpdates(currentTime)) {
    statusLED.showInfo();  // Show checking for updates
    otaUpdater.checkForUpdates();
    config.updateUpdateCheckTime(currentTime);
    statusLED.showNormal();  // Back to normal
  }

  delay(ConfigConstants::Timing::MAIN_LOOP_DELAY);
}

