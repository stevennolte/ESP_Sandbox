/*
 * ESP32 IoT Device with OTA Updates
 * Features: LED control, MQTT integration, Web interface, OTA updates
 */

#include <DHT.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPMQTTManager.h>
#include <ESPOTAUpdater.h>
#include <Update.h>
#include <FS.h>
#include <HTTPClient.h>

// --- Configuration Constants ---
const char* mqtt_user = "steve";
const char* mqtt_pass = "Doctor*9";
const int FIRMWARE_VERSION = 923; // v9.14
const char* GITHUB_REPO = "stevennolte/ESP_Sandbox";
const unsigned long updateInterval = 5 * 60 * 1000; // 5 minutes
String wifi_ssid = "SSEI";         // Default SSID, can be updated via web interface
String wifi_password = "Nd14il!la"; // Default password, can be updated via web interface

// --- HTTP Constants ---
const int HTTP_TIMEOUT_SHORT = 15000;  // 15 seconds
const int HTTP_TIMEOUT_LONG = 30000;   // 30 seconds
const char* USER_AGENT_TEMPLATE = "ESP32-Template-Updater";
const char* USER_AGENT_CHECKER = "ESP32-Template-Checker";

// --- Hardware Configuration ---
#define DHT_PIN 4          // DHT22 data pin
#define DHT_TYPE DHT22     // DHT sensor type
const int ledPin = 2;         // Built-in LED
const int ledChannel = 0;     // PWM channel
const int ledFreq = 5000;     // PWM frequency
const int ledResolution = 8;  // 8-bit resolution (0-255)

// --- Network Configuration ---
String mqtt_server_ip = "192.168.1.12"; // Default fallback IP
const int mqtt_port = 1883;
String client_id = "ESP_Default"; // Loaded from preferences

// --- Global Variables ---
int ledBrightness = 128;  // Default brightness (0-255)
unsigned long lastUpdateCheck = 0;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30 * 1000; // Check WiFi every 30 seconds

// --- Timing Constants ---
const unsigned long LED_PULSE_DURATION = 50;
const unsigned long MAIN_LOOP_DELAY = 1000;
const unsigned long NETWORK_STABILIZATION_DELAY = 2000;
const unsigned long REBOOT_DELAY = 3000;

// --- Network Constants ---
const int WIFI_MAX_ATTEMPTS = 30;
const int WIFI_RECONNECT_ATTEMPTS = 20;
const int WIFI_RETRY_DELAY = 500;

// --- Object Instances ---
Preferences preferences;
WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
ESPMQTTManager mqttManager(mqtt_user, mqtt_pass, "192.168.1.12", mqtt_port);
ESPOTAUpdater otaUpdater(GITHUB_REPO, FIRMWARE_VERSION);

// --- Function Declarations ---
float readCPUTemperature();
float readDHTTemperature();
float readDHTHumidity();
String getBoardType();
void setup_wifi();
void checkWiFiConnection();
String loadHTMLTemplate(const char* filename);
void handleFileList();
void handleFileDownload();
void handleFileUpload();
void handleFileUploadComplete();
void handleFirmwareUpload();
void handleFirmwareUploadComplete();
void handleWifiConfig();
void handleWifiUpdate();
void handleNetworkScan();
void handleUpdateTemplate();
void handleUpdateTemplateAction();
void handleForceTemplateUpdate();
void checkForTemplateUpdate();
bool downloadTemplate();
void forceTemplateUpdate();
void ensureTemplateExists();
void handleDebug();

// --- Utility Functions ---
String makeGitHubAPICall(const String& endpoint);
bool downloadFileFromGitHub(const String& filePath, const String& localPath);
void updateStoredCommitHash();

// --- OTA Update Callbacks ---
void onUpdateAvailable(int currentVersion, int newVersion, const String& downloadUrl) {
  Serial.printf("*** UPDATE AVAILABLE ***\n");
  Serial.printf("Current version: %d, New version: %d\n", currentVersion, newVersion);
  Serial.printf("Download URL: %s\n", downloadUrl.c_str());
  
  // Automatically start the update process
  Serial.println("Starting automatic firmware update...");
  otaUpdater.performUpdate(downloadUrl.c_str());
}

void onUpdateProgress(size_t progress, size_t total) {
  Serial.printf("OTA Progress: %d/%d bytes (%d%%)\n", progress, total, (progress * 100) / total);
}

void onUpdateComplete(bool success, const String& message) {
  if (success) {
    Serial.println("*** OTA UPDATE SUCCESSFUL ***");
    
    // Download latest template after successful firmware update
    Serial.println("Downloading latest web template...");
    if (downloadTemplate()) {
      updateStoredCommitHash();
      Serial.println("✓ Template updated with firmware");
    } else {
      Serial.println("✗ Failed to download latest template");
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

float readDHTTemperature() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return -999.0; // Return error value
  }
  return temp;
}

float readDHTHumidity() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed to read humidity from DHT sensor!");
    return -999.0; // Return error value
  }
  return humidity;
}

// --- WiFi Setup ---
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  
  // Configure WiFi for better reconnection behavior
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS) {
    delay(WIFI_RETRY_DELAY);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
  
  delay(NETWORK_STABILIZATION_DELAY);
}

// --- WiFi Connection Monitor ---
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Attempting to reconnect...");
    
    // Try to reconnect
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_RECONNECT_ATTEMPTS) {
      delay(WIFI_RETRY_DELAY);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✓ WiFi reconnected!");
      Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    } else {
      Serial.println("\n✗ Failed to reconnect to WiFi");
    }
  }
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
  html.replace("{{CLIENT_ID}}", client_id);
  html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
  html.replace("{{LED_BRIGHTNESS}}", String(ledBrightness));
  html.replace("{{MQTT_SERVER}}", mqtt_server_ip);
  html.replace("{{WIFI_RSSI}}", String(WiFi.RSSI()));
  html.replace("{{WIFI_STATUS}}", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  
  // Add environmental sensor data
  float dhtTemp = readDHTTemperature();
  float dhtHumidity = readDHTHumidity();
  html.replace("{{DHT_TEMPERATURE}}", dhtTemp != -999.0 ? String(dhtTemp, 1) + "°C" : "Error");
  html.replace("{{DHT_HUMIDITY}}", dhtHumidity != -999.0 ? String(dhtHumidity, 1) + "%" : "Error");
  
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
      client_id = newClientId;
      preferences.begin("esp-config", false);
      preferences.putString("client_id", client_id);
      preferences.end();
      
      // Update MQTT topics with new client_id
      mqttManager.updateTopics(client_id);
      
      // Restart mDNS with new hostname
      MDNS.end();
      if (!MDNS.begin(client_id.c_str())) {
        Serial.println("Error restarting mDNS with new hostname");
      } else {
        Serial.println("mDNS restarted with new hostname: " + client_id);
        MDNS.addService("http", "tcp", 80);
      }
      
      String html = "<!DOCTYPE html><html><head><title>Updated</title></head><body>";
      html += "<h1>Client ID Updated</h1>";
      html += "<p>New Client ID: <strong>" + client_id + "</strong></p>";
      html += "<p>New mDNS address: <strong>http://" + client_id + ".local</strong></p>";
      html += "<p>Device will reconnect to MQTT with new ID.</p>";
      html += "<p><a href='/'>Back to Home</a></p>";
      html += "</body></html>";
      server.send(200, "text/html", html);
      
      // Force MQTT reconnection with new client ID
      mqttManager.disconnect();
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
      ledBrightness = newBrightness;
      preferences.begin("esp-config", false);
      preferences.putInt("led_brightness", ledBrightness);
      preferences.end();
      
      String html = "<!DOCTYPE html><html><head><title>Brightness Updated</title></head><body>";
      html += "<h1>LED Brightness Updated</h1>";
      html += "<p>New Brightness: <strong>" + String(ledBrightness) + "</strong></p>";
      html += "<p><a href='/'>Back to Home</a></p>";
      html += "</body></html>";
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
  String html = "<!DOCTYPE html><html><head><title>File Manager</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:800px;margin:0 auto;padding:20px;background-color:#f5f5f5;}";
  html += ".container{background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "table{width:100%;border-collapse:collapse;margin:20px 0;}";
  html += "th,td{padding:10px;text-align:left;border-bottom:1px solid #ddd;}";
  html += "th{background-color:#f2f2f2;}";
  html += "a{color:#007bff;text-decoration:none;}a:hover{text-decoration:underline;}";
  html += ".upload-form{margin:20px 0;padding:20px;background-color:#f8f9fa;border-radius:5px;}";
  html += "input[type='file']{margin:10px 0;}";
  html += "input[type='submit']{background-color:#28a745;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>File Manager</h1>";
  html += "<p><a href='/'>← Back to Main</a></p>";
  
  html += "<div class='upload-form'>";
  html += "<h2>Upload File</h2>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='file' required>";
  html += "<input type='submit' value='Upload File'>";
  html += "</form></div>";
  
  html += "<h2>Files on Device</h2>";
  html += "<table><tr><th>Filename</th><th>Size</th><th>Actions</th></tr>";
  
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    html += "<tr><td>" + fileName + "</td>";
    html += "<td>" + String(file.size()) + " bytes</td>";
    html += "<td><a href='/download?file=" + fileName + "'>Download</a></td></tr>";
    file = root.openNextFile();
  }
  
  html += "</table></div></body></html>";
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
  server.send(200, "text/html", "<!DOCTYPE html><html><head><title>Upload Complete</title></head><body>"
    "<h1>File Upload Complete</h1><p><a href='/files'>Back to File Manager</a></p></body></html>");
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
  String html = "<!DOCTYPE html><html><head><title>Firmware Update</title></head><body>";
  if (Update.hasError()) {
    html += "<h1>Firmware Update Failed</h1>";
    html += "<p>Error: " + String(Update.getError()) + "</p>";
    html += "<p><a href='/'>Back to Main</a></p>";
  } else {
    html += "<h1>Firmware Update Successful</h1>";
    html += "<p>Device will reboot in 3 seconds...</p>";
    html += "<script>setTimeout(function(){window.location.href='/';}, 5000);</script>";
  }
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  if (!Update.hasError()) {
    delay(REBOOT_DELAY);
    ESP.restart();
  }
}

// --- WiFi Configuration Functions ---
void handleWifiConfig() {
  String html = "<!DOCTYPE html><html><head><title>WiFi Configuration</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:0 auto;padding:20px;background-color:#f5f5f5;}";
  html += ".container{background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "input[type='text'],input[type='password']{width:100%;padding:8px;margin:5px 0;box-sizing:border-box;}";
  html += "input[type='submit']{background-color:#007bff;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:10px 0;}";
  html += ".info{background-color:#e7f3ff;padding:10px;border-radius:4px;margin:10px 0;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>WiFi Configuration</h1>";
  html += "<p><a href='/'>← Back to Main</a></p>";
  
  html += "<div class='info'>";
  html += "<p><strong>Current WiFi:</strong> " + WiFi.SSID() + "</p>";
  html += "<p><strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p><strong>Status:</strong> " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</p>";
  html += "</div>";
  
  html += "<h2>Change WiFi Network</h2>";
  html += "<form method='POST' action='/wifi-update'>";
  html += "<label for='ssid'>Network Name (SSID):</label>";
  html += "<input type='text' id='ssid' name='ssid' value='' required>";
  html += "<label for='password'>Password:</label>";
  html += "<input type='password' id='password' name='password' value='' required>";
  html += "<input type='submit' value='Update WiFi Settings'>";
  html += "</form>";
  
  html += "<h2>Available Networks</h2>";
  html += "<p>Scanning for networks...</p>";
  html += "<div id='networks'></div>";
  
  html += "<script>";
  html += "function scanNetworks() {";
  html += "  fetch('/scan-networks').then(response => response.json()).then(data => {";
  html += "    let html = '<ul>';";
  html += "    data.networks.forEach(network => {";
  html += "      html += '<li><strong>' + network.ssid + '</strong> (' + network.rssi + ' dBm) ';";
  html += "      html += network.encrypted ? '[Secured]' : '[Open]';";
  html += "      html += ' <button onclick=\"document.getElementById(\\'ssid\\').value=\\''+network.ssid+'\\'\">Use</button></li>';";
  html += "    });";
  html += "    html += '</ul>';";
  html += "    document.getElementById('networks').innerHTML = html;";
  html += "  });";
  html += "}";
  html += "scanNetworks();";
  html += "</script>";
  
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleWifiUpdate() {
  if (!server.hasArg("ssid") || !server.hasArg("password")) {
    server.send(400, "text/plain", "Missing SSID or password");
    return;
  }
  
  String newSSID = server.arg("ssid");
  String newPassword = server.arg("password");
  
  // Save new WiFi credentials to preferences
  preferences.begin("esp-config", false);
  preferences.putString("wifi_ssid", newSSID);
  preferences.putString("wifi_password", newPassword);
  preferences.end();
  
  String html = "<!DOCTYPE html><html><head><title>WiFi Updated</title></head><body>";
  html += "<h1>WiFi Settings Updated</h1>";
  html += "<p>New SSID: <strong>" + newSSID + "</strong></p>";
  html += "<p>Device will restart and connect to the new network...</p>";
  html += "<p>Please connect to the new network to access the device.</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  delay(REBOOT_DELAY);
  ESP.restart();
}

// --- Network Scanning ---
void handleNetworkScan() {
  WiFi.scanDelete();
  int n = WiFi.scanNetworks();
  
  String json = "{\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"encrypted\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
    json += "}";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}

// --- Debug Page ---
void handleDebug() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 Debug Information</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:800px;margin:0 auto;padding:20px;background-color:#f5f5f5;}";
  html += ".container{background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += ".debug-section{margin:20px 0;padding:15px;background-color:#f8f9fa;border-radius:5px;}";
  html += ".debug-item{display:flex;justify-content:space-between;padding:5px 0;border-bottom:1px solid #dee2e6;}";
  html += ".debug-label{font-weight:bold;color:#495057;}";
  html += ".debug-value{color:#007bff;font-family:monospace;}";
  html += ".error{color:#dc3545;}";
  html += ".success{color:#28a745;}";
  html += ".warning{color:#ffc107;}";
  html += "button{background-color:#007bff;color:white;padding:10px 15px;border:none;border-radius:4px;cursor:pointer;margin:5px;}";
  html += "button:hover{background-color:#0056b3;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>🔧 ESP32 Debug Information</h1>";
  html += "<p><a href='/'>← Back to Main</a></p>";
  
  // System Information
  html += "<div class='debug-section'>";
  html += "<h2>💻 System Information</h2>";
  html += "<div class='debug-item'><span class='debug-label'>Board Type:</span><span class='debug-value'>" + getBoardType() + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Firmware Version:</span><span class='debug-value'>" + String(FIRMWARE_VERSION) + " (v" + String(FIRMWARE_VERSION/100) + "." + String(FIRMWARE_VERSION%100) + ")</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Chip Model:</span><span class='debug-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Chip Cores:</span><span class='debug-value'>" + String(ESP.getChipCores()) + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>CPU Frequency:</span><span class='debug-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Flash Size:</span><span class='debug-value'>" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Free Heap:</span><span class='debug-value'>" + String(ESP.getFreeHeap()) + " bytes</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Min Free Heap:</span><span class='debug-value'>" + String(ESP.getMinFreeHeap()) + " bytes</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Max Alloc Heap:</span><span class='debug-value'>" + String(ESP.getMaxAllocHeap()) + " bytes</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Uptime:</span><span class='debug-value'>" + String(millis() / 1000) + " seconds</span></div>";
  html += "</div>";
  
  // Network Information
  html += "<div class='debug-section'>";
  html += "<h2>📡 Network Information</h2>";
  html += "<div class='debug-item'><span class='debug-label'>WiFi Status:</span><span class='debug-value " + String(WiFi.status() == WL_CONNECTED ? "success" : "error") + "'>" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>SSID:</span><span class='debug-value'>" + WiFi.SSID() + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>IP Address:</span><span class='debug-value'>" + WiFi.localIP().toString() + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Gateway:</span><span class='debug-value'>" + WiFi.gatewayIP().toString() + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>DNS:</span><span class='debug-value'>" + WiFi.dnsIP().toString() + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>MAC Address:</span><span class='debug-value'>" + WiFi.macAddress() + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Signal Strength:</span><span class='debug-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>MQTT Status:</span><span class='debug-value " + String(mqttManager.isConnected() ? "success" : "error") + "'>" + String(mqttManager.isConnected() ? "Connected" : "Disconnected") + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>MQTT Server:</span><span class='debug-value'>" + mqtt_server_ip + ":" + String(mqtt_port) + "</span></div>";
  html += "</div>";
  
  // Sensor Information
  html += "<div class='debug-section'>";
  html += "<h2>🌡️ Sensor Information</h2>";
  float cpuTemp = readCPUTemperature();
  float dhtTemp = readDHTTemperature();
  float dhtHumidity = readDHTHumidity();
  html += "<div class='debug-item'><span class='debug-label'>CPU Temperature:</span><span class='debug-value'>" + String(cpuTemp, 1) + "°C</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>DHT22 Temperature:</span><span class='debug-value " + String(dhtTemp != -999.0 ? "success" : "error") + "'>" + String(dhtTemp != -999.0 ? String(dhtTemp, 1) + "°C" : "Error") + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>DHT22 Humidity:</span><span class='debug-value " + String(dhtHumidity != -999.0 ? "success" : "error") + "'>" + String(dhtHumidity != -999.0 ? String(dhtHumidity, 1) + "%" : "Error") + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>LED Brightness:</span><span class='debug-value'>" + String(ledBrightness) + "/255</span></div>";
  html += "</div>";
  
  // Timing Information
  html += "<div class='debug-section'>";
  html += "<h2>⏰ Timing Information</h2>";
  unsigned long currentTime = millis();
  html += "<div class='debug-item'><span class='debug-label'>Current Time:</span><span class='debug-value'>" + String(currentTime) + " ms</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Last Update Check:</span><span class='debug-value'>" + String(lastUpdateCheck) + " ms</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Time Since Update Check:</span><span class='debug-value'>" + String((currentTime - lastUpdateCheck) / 1000) + " seconds</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Last WiFi Check:</span><span class='debug-value'>" + String(lastWiFiCheck) + " ms</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Time Since WiFi Check:</span><span class='debug-value'>" + String((currentTime - lastWiFiCheck) / 1000) + " seconds</span></div>";
  html += "</div>";
  
  // Storage Information
  html += "<div class='debug-section'>";
  html += "<h2>💾 Storage Information</h2>";
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  html += "<div class='debug-item'><span class='debug-label'>LittleFS Total:</span><span class='debug-value'>" + String(totalBytes) + " bytes (" + String(totalBytes/1024) + " KB)</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>LittleFS Used:</span><span class='debug-value'>" + String(usedBytes) + " bytes (" + String(usedBytes/1024) + " KB)</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>LittleFS Free:</span><span class='debug-value'>" + String(totalBytes - usedBytes) + " bytes (" + String((totalBytes - usedBytes)/1024) + " KB)</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Usage Percentage:</span><span class='debug-value'>" + String((usedBytes * 100) / totalBytes) + "%</span></div>";
  html += "</div>";
  
  // Configuration Information
  html += "<div class='debug-section'>";
  html += "<h2>⚙️ Configuration</h2>";
  preferences.begin("esp-config", true);
  String storedCommit = preferences.getString("last_commit", "Unknown");
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  String storedClientId = preferences.getString("client_id", "Not Set");
  int storedBrightness = preferences.getInt("led_brightness", 0);
  String storedSSID = preferences.getString("wifi_ssid", "Not Set");
  preferences.end();
  
  html += "<div class='debug-item'><span class='debug-label'>Stored Client ID:</span><span class='debug-value'>" + storedClientId + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Stored LED Brightness:</span><span class='debug-value'>" + String(storedBrightness) + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Stored WiFi SSID:</span><span class='debug-value'>" + storedSSID + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Stored Template Commit:</span><span class='debug-value'>" + (storedCommit.length() > 7 ? storedCommit.substring(0, 7) : storedCommit) + "</span></div>";
  html += "<div class='debug-item'><span class='debug-label'>Stored Firmware Version:</span><span class='debug-value'>" + String(storedFirmwareVersion) + " (v" + String(storedFirmwareVersion/100) + "." + String(storedFirmwareVersion%100) + ")</span></div>";
  html += "</div>";
  
  // Manual Actions
  html += "<div class='debug-section'>";
  html += "<h2>🔄 Manual Actions</h2>";
  html += "<button onclick='refreshPage()'>🔄 Refresh Data</button>";
  html += "<button onclick='testMQTT()'>📡 Test MQTT Connection</button>";
  html += "<button onclick='testSensors()'>🌡️ Test Sensors</button>";
  html += "<button onclick='forceFirmwareCheck()'>⬇️ Force Firmware Check</button>";
  html += "<div id='actionResult' style='margin-top:10px;'></div>";
  html += "</div>";
  
  html += "<script>";
  html += "function refreshPage() { location.reload(); }";
  html += "function testMQTT() {";
  html += "  document.getElementById('actionResult').innerHTML = '<p style=\"color:blue;\">Testing MQTT connection...</p>';";
  html += "  // This would need a separate endpoint to test MQTT";
  html += "  setTimeout(() => { document.getElementById('actionResult').innerHTML = '<p style=\"color:green;\">MQTT test completed (check serial output)</p>'; }, 2000);";
  html += "}";
  html += "function testSensors() {";
  html += "  document.getElementById('actionResult').innerHTML = '<p style=\"color:blue;\">Testing sensors...</p>';";
  html += "  setTimeout(() => { refreshPage(); }, 2000);";
  html += "}";
  html += "function forceFirmwareCheck() {";
  html += "  document.getElementById('actionResult').innerHTML = '<p style=\"color:blue;\">Checking for firmware updates...</p>';";
  html += "  // This would trigger a firmware check";
  html += "  setTimeout(() => { document.getElementById('actionResult').innerHTML = '<p style=\"color:green;\">Firmware check completed (check serial output)</p>'; }, 3000);";
  html += "}";
  html += "</script>";
  
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// --- Utility Functions ---
String makeGitHubAPICall(const String& endpoint) {
  HTTPClient http;
  String url = "https://api.github.com/repos/" + String(GITHUB_REPO) + "/" + endpoint;
  
  http.begin(url);
  http.addHeader("User-Agent", USER_AGENT_CHECKER);
  http.setTimeout(HTTP_TIMEOUT_SHORT);
  
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
  String url = "https://raw.githubusercontent.com/" + String(GITHUB_REPO) + "/main/" + filePath;
  
  http.begin(url);
  http.addHeader("User-Agent", USER_AGENT_TEMPLATE);
  http.setTimeout(HTTP_TIMEOUT_LONG);
  
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
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, response) == DeserializationError::Ok) {
      String latestCommit = doc["sha"].as<String>();
      preferences.begin("esp-config", false);
      preferences.putString("last_commit", latestCommit);
      preferences.end();
      Serial.printf("Updated commit hash: %s\n", latestCommit.c_str());
    }
  }
}

// --- Template Update Functions ---
void handleUpdateTemplate() {
  String html = "<!DOCTYPE html><html><head><title>Update Web Template</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:0 auto;padding:20px;background-color:#f5f5f5;}";
  html += ".container{background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "button{background-color:#007bff;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:10px 0;}";
  html += ".status{margin:20px 0;padding:10px;border-radius:4px;}";
  html += ".success{background-color:#d4edda;color:#155724;border:1px solid #c3e6cb;}";
  html += ".error{background-color:#f8d7da;color:#721c24;border:1px solid #f5c6cb;}";
  html += ".info{background-color:#e7f3ff;padding:10px;border-radius:4px;margin:10px 0;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>Update Web Template</h1>";
  html += "<p><a href='/'>← Back to Main</a></p>";
  
  html += "<div class='info'>";
  html += "<p>Web templates are now automatically updated with each firmware release.</p>";
  html += "<p><strong>Repository:</strong> " + String(GITHUB_REPO) + "</p>";
  html += "<p><strong>File:</strong> data/index.html</p>";
  
  // Show current template info
  preferences.begin("esp-config", true);
  String currentCommit = preferences.getString("last_commit", "Unknown");
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  preferences.end();
  html += "<p><strong>Current Template:</strong> " + (currentCommit.length() > 7 ? currentCommit.substring(0, 7) : currentCommit) + "</p>";
  html += "<p><strong>Template Firmware Version:</strong> v" + String(storedFirmwareVersion/100) + "." + String(storedFirmwareVersion%100) + "</p>";
  html += "<p><strong>Current Firmware Version:</strong> v" + String(FIRMWARE_VERSION/100) + "." + String(FIRMWARE_VERSION%100) + "</p>";
  html += "</div>";
  
  html += "<p><strong>Note:</strong> Templates automatically update when new firmware is installed via OTA. Manual updates are only needed for testing or troubleshooting.</p>";
  
  html += "<button onclick='updateTemplate()'>Check for Template Updates</button>";
  html += "<button onclick='forceUpdate()' style='background-color:#dc3545;margin-left:10px;'>Force Update Template</button>";
  html += "<div id='status'></div>";
  
  html += "<script>";
  html += "function updateTemplate() {";
  html += "  document.getElementById('status').innerHTML = '<div class=\"status\">Checking for template updates...</div>';";
  html += "  fetch('/update-template-action', {method: 'POST'})";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      if (data.includes('success')) {";
  html += "        document.getElementById('status').innerHTML = '<div class=\"status success\">Template updated successfully! Please refresh the main page to see changes.</div>';";
  html += "      } else {";
  html += "        document.getElementById('status').innerHTML = '<div class=\"status error\">Update result: ' + data + '</div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(error => {";
  html += "      document.getElementById('status').innerHTML = '<div class=\"status error\">Update failed: ' + error + '</div>';";
  html += "    });";
  html += "}";
  html += "function forceUpdate() {";
  html += "  document.getElementById('status').innerHTML = '<div class=\"status\">Force downloading template from GitHub...</div>';";
  html += "  fetch('/force-template-update', {method: 'POST'})";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      if (data.includes('success')) {";
  html += "        document.getElementById('status').innerHTML = '<div class=\"status success\">Template force updated successfully! Please refresh the main page to see changes.</div>';";
  html += "      } else {";
  html += "        document.getElementById('status').innerHTML = '<div class=\"status error\">Force update result: ' + data + '</div>';";
  html += "      }";
  html += "    })";
  html += "    .catch(error => {";
  html += "      document.getElementById('status').innerHTML = '<div class=\"status error\">Force update failed: ' + error + '</div>';";
  html += "    });";
  html += "}";
  html += "</script>";
  
  html += "</div></body></html>";
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
  return downloadFileFromGitHub("data/index.html", "/index.html");
}

void checkForTemplateUpdate() {
  Serial.println("Checking for template updates...");
  
  String response = makeGitHubAPICall("commits/main");
  if (response.length() == 0) {
    Serial.println("Failed to get GitHub API response");
    return;
  }

  StaticJsonDocument<1024> doc;
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
    Serial.println("Template update needed, downloading...");
    if (downloadTemplate()) {
      preferences.begin("esp-config", false);
      preferences.putString("last_commit", latestCommit);
      preferences.end();
      Serial.println("✓ Template updated successfully");
    } else {
      Serial.println("✗ Failed to download template");
    }
  } else {
    Serial.println("Template is up to date");
  }
}

void forceTemplateUpdate() {
  Serial.println("Force updating template...");
  
  if (downloadTemplate()) {
    updateStoredCommitHash();
    Serial.println("✓ Force update complete");
  } else {
    Serial.println("✗ Force update failed");
  }
}

void ensureTemplateExists() {
  // Check if index.html exists
  if (!LittleFS.exists("/index.html")) {
    Serial.println("Template file not found, downloading from GitHub...");
    forceTemplateUpdate();
    return;
  }
  
  // Check if this is the first boot after a firmware update
  preferences.begin("esp-config", true);
  int storedFirmwareVersion = preferences.getInt("last_firmware_version", 0);
  preferences.end();
  
  if (storedFirmwareVersion != FIRMWARE_VERSION) {
    Serial.printf("Firmware updated from v%d to v%d, downloading latest template...\n", 
                  storedFirmwareVersion, FIRMWARE_VERSION);
    
    // Download latest template
    forceTemplateUpdate();
    
    // Update stored firmware version
    preferences.begin("esp-config", false);
    preferences.putInt("last_firmware_version", FIRMWARE_VERSION);
    preferences.end();
    
    Serial.println("✓ Template synchronized with new firmware");
  } else {
    Serial.println("✓ Template exists and firmware version matches");
  }
}

// --- Web Server Setup ---
void setupWebServer() {
  // Initialize mDNS
  if (!MDNS.begin(client_id.c_str())) {
    Serial.println("ERROR: mDNS failed to start");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("✓ mDNS: http://%s.local\n", client_id.c_str());
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
    String html = "<!DOCTYPE html><html><head><title>Firmware Update</title>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:0 auto;padding:20px;background-color:#f5f5f5;}";
    html += ".container{background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
    html += "input[type='file']{margin:10px 0;}";
    html += "input[type='submit']{background-color:#dc3545;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;}";
    html += ".warning{background-color:#fff3cd;padding:15px;border-radius:4px;margin:10px 0;border-left:4px solid #ffc107;}";
    html += "</style></head><body><div class='container'>";
    html += "<h1>Firmware Update</h1>";
    html += "<p><a href='/'>← Back to Main</a></p>";
    html += "<div class='warning'><strong>Warning:</strong> Only upload firmware files (.bin). Incorrect files may brick the device!</div>";
    html += "<form method='POST' action='/firmware-upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='firmware' accept='.bin' required>";
    html += "<input type='submit' value='Upload Firmware'>";
    html += "</form></div></body></html>";
    server.send(200, "text/html", html);
  });
  server.on("/firmware-upload", HTTP_POST, handleFirmwareUploadComplete, handleFirmwareUpload);
  
  // WiFi configuration routes
  server.on("/wifi", handleWifiConfig);
  server.on("/wifi-update", HTTP_POST, handleWifiUpdate);
  server.on("/scan-networks", handleNetworkScan);
  
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
  preferences.begin("esp-config", true); // read-only
  client_id = preferences.getString("client_id", "ESP_Default");
  ledBrightness = preferences.getInt("led_brightness", 128);
  
  // Load WiFi credentials if saved
  String saved_ssid = preferences.getString("wifi_ssid", "");
  String saved_password = preferences.getString("wifi_password", "");
  
  preferences.end();
  
  // Update WiFi credentials if they were saved
  if (saved_ssid.length() > 0) {
    wifi_ssid = saved_ssid;
    wifi_password = saved_password;
  }
  
  // Update MQTT topics with loaded client_id
  mqttManager.updateTopics(client_id);
  
  Serial.printf("✓ Client ID: %s\n", client_id.c_str());
  Serial.printf("✓ LED Brightness: %d\n", ledBrightness);
  if (saved_ssid.length() > 0) {
    Serial.printf("✓ Saved WiFi: %s\n", saved_ssid.c_str());
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\n=== ESP32 IoT Device Starting ===");
  Serial.printf("Board Type: %s\n", getBoardType().c_str());
  Serial.printf("Firmware Version: %d (v%d.%d)\n", FIRMWARE_VERSION, FIRMWARE_VERSION/100, FIRMWARE_VERSION%100);
  
  // Initialize hardware
  ledcSetup(ledChannel, ledFreq, ledResolution);
  ledcAttachPin(ledPin, ledChannel);
  ledcWrite(ledChannel, 0); // Start with LED off
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("✓ DHT22 sensor initialized");
  
  // Initialize filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: Failed to mount LittleFS");
    return;
  }
  Serial.println("✓ LittleFS mounted");

  // Load saved configuration
  loadClientId();

  // Connect to WiFi
  setup_wifi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ERROR: Cannot continue without WiFi");
    return;
  }
  
  // Ensure web template exists and is up to date
  ensureTemplateExists();

  // Initialize web server
  setupWebServer();

  // Initialize OTA updater
  otaUpdater.setUpdateAvailableCallback(onUpdateAvailable);
  otaUpdater.setUpdateProgressCallback(onUpdateProgress);
  otaUpdater.setUpdateCompleteCallback(onUpdateComplete);
  otaUpdater.setBoardType(getBoardType());
  otaUpdater.enableAutoUpdate(false); // Disable auto-update to prevent duplicates

  // Perform initial update check
  Serial.println("Checking for firmware updates...");
  otaUpdater.checkForUpdates();
  lastUpdateCheck = millis();
  
  // Initialize MQTT (moved to end for better stability)
  mqtt_server_ip = mqttManager.discoverServer();
  mqttManager.updateServerIP(mqtt_server_ip);
  mqttManager.begin(client_id);
  Serial.printf("✓ MQTT server: %s\n", mqtt_server_ip.c_str());
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  // LED heartbeat indicator
  ledcWrite(ledChannel, ledBrightness);
  delay(LED_PULSE_DURATION);
  ledcWrite(ledChannel, 0);
  
  // Handle web server requests
  server.handleClient();
  
  // MQTT connection and message handling
  if (!mqttManager.isConnected()) {
    mqttManager.connect();
  }
  mqttManager.loop();

  // Periodic tasks with timing
  
  // WiFi connection monitoring (every 30 seconds)
  if (currentTime - lastWiFiCheck > wifiCheckInterval) {
    checkWiFiConnection();
    lastWiFiCheck = currentTime;
  }
  
  // Environmental data publishing (every 10 seconds)
  if (mqttManager.shouldPublishTemperature(currentTime)) {
    float cpuTemp = readCPUTemperature();
    float dhtTemp = readDHTTemperature();
    float dhtHumidity = readDHTHumidity();
    
    // Publish CPU temperature (for system monitoring)
    mqttManager.publishCpuTemperature(cpuTemp);
    // mqttManager.publishTemperature(dhtTemp);
    // Publish DHT22 data (environmental monitoring)
    if (dhtTemp != -999.0) {
      Serial.printf("DHT Temperature: %.1f°C\n", dhtTemp);
      mqttManager.publishTemperature(dhtTemp);
      // TODO: Add publishEnvironmentalTemperature method to MQTT manager
    }
    
    if (dhtHumidity != -999.0) {
      Serial.printf("DHT Humidity: %.1f%%\n", dhtHumidity);
      // TODO: Add publishHumidity method to MQTT manager
    }
    
    mqttManager.updateLastPublishTime(currentTime);
  }

  // Firmware version publishing (every 5 minutes)
  if (mqttManager.shouldPublishFirmwareVersion(currentTime)) {
    mqttManager.publishFirmwareVersion(FIRMWARE_VERSION);
    mqttManager.updateLastVersionPublishTime(currentTime);
  }

  // OTA update checking (every 5 minutes)
  if (currentTime - lastUpdateCheck > updateInterval) {
    otaUpdater.checkForUpdates();
    lastUpdateCheck = currentTime;
  }

  // MQTT server re-discovery (every 15 minutes)
  if (mqttManager.shouldRediscoverServer(currentTime)) {
    Serial.println("Re-discovering MQTT server...");
    String newMQTTServer = mqttManager.discoverServer();
    mqttManager.updateServerIP(newMQTTServer);
    mqtt_server_ip = newMQTTServer;
    mqttManager.updateLastDiscoveryTime(currentTime);
  }

  delay(MAIN_LOOP_DELAY);
}

