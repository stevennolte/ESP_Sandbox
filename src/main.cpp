/*
 * ESP32 IoT Device with OTA Updates
 * Features: LED control, MQTT integration, Web interface, OTA updates
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ESPMQTTManager.h>
#include <ESPOTAUpdater.h>

// --- Configuration Constants ---
const char* mqtt_user = "steve";
const char* mqtt_pass = "Doctor*9";
const int FIRMWARE_VERSION = 95; // v9.5
const char* GITHUB_REPO = "stevennolte/ESP_Sandbox";
const unsigned long updateInterval = 5 * 60 * 1000; // 5 minutes
const char* ssid = "SSEI";
const char* password = "Nd14il!la";

// --- Hardware Configuration ---
const int oneWireBus = 4;     // DS18B20 data pin
const int powerPin = 21;      // DS18B20 power pin
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

// --- Object Instances ---
Preferences preferences;
WebServer server(80);
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
ESPMQTTManager mqttManager(mqtt_user, mqtt_pass, "192.168.1.12", mqtt_port);
ESPOTAUpdater otaUpdater(GITHUB_REPO, FIRMWARE_VERSION);

// --- Function Declarations ---
float readCPUTemperature();
String getBoardType();
void setup_wifi();
String loadHTMLTemplate(const char* filename);

// --- OTA Update Callbacks ---
void onUpdateAvailable(int currentVersion, int newVersion, const String& downloadUrl) {
  Serial.printf("*** UPDATE AVAILABLE ***\n");
  Serial.printf("Current version: %d, New version: %d\n", currentVersion, newVersion);
  Serial.printf("Download URL: %s\n", downloadUrl.c_str());
}

void onUpdateProgress(size_t progress, size_t total) {
  Serial.printf("OTA Progress: %d/%d bytes (%d%%)\n", progress, total, (progress * 100) / total);
}

void onUpdateComplete(bool success, const String& message) {
  if (success) {
    Serial.println("*** OTA UPDATE SUCCESSFUL ***");
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

// --- WiFi Setup ---
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
  
  delay(2000); // Network stabilization
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
  
  server.begin();
  Serial.printf("✓ Web server: http://%s\n", WiFi.localIP().toString().c_str());
}

// --- Configuration Management ---
void loadClientId() {
  preferences.begin("esp-config", true); // read-only
  client_id = preferences.getString("client_id", "ESP_Default");
  ledBrightness = preferences.getInt("led_brightness", 128);
  preferences.end();
  
  // Update MQTT topics with loaded client_id
  mqttManager.updateTopics(client_id);
  
  Serial.printf("✓ Client ID: %s\n", client_id.c_str());
  Serial.printf("✓ LED Brightness: %d\n", ledBrightness);
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("\n=== ESP32 IoT Device Starting ===");
  Serial.printf("Board Type: %s\n", getBoardType().c_str());
  Serial.printf("Firmware Version: %d (v%d.%d)\n", FIRMWARE_VERSION, FIRMWARE_VERSION/10, FIRMWARE_VERSION%10);
  
  // Initialize hardware
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, HIGH); // Power on DS18B20 sensor
  
  ledcSetup(ledChannel, ledFreq, ledResolution);
  ledcAttachPin(ledPin, ledChannel);
  ledcWrite(ledChannel, 0); // Start with LED off
  
  sensors.begin();
  
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
  
  // Initialize MQTT
  mqtt_server_ip = mqttManager.discoverServer();
  mqttManager.updateServerIP(mqtt_server_ip);
  mqttManager.begin(client_id);
  Serial.printf("✓ MQTT server: %s\n", mqtt_server_ip.c_str());

  // Initialize web server
  setupWebServer();

  // Initialize OTA updater
  otaUpdater.setUpdateAvailableCallback(onUpdateAvailable);
  otaUpdater.setUpdateProgressCallback(onUpdateProgress);
  otaUpdater.setUpdateCompleteCallback(onUpdateComplete);
  otaUpdater.setBoardType(getBoardType());
  otaUpdater.enableAutoUpdate(true);

  // Perform initial update check
  Serial.println("Checking for firmware updates...");
  otaUpdater.checkForUpdates();
  lastUpdateCheck = millis();
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  // LED heartbeat indicator
  ledcWrite(ledChannel, ledBrightness);
  delay(50);
  ledcWrite(ledChannel, 0);
  
  // Handle web server requests
  server.handleClient();
  
  // MQTT connection and message handling
  if (!mqttManager.isConnected()) {
    mqttManager.connect();
  }
  mqttManager.loop();

  // Periodic tasks with timing
  
  // CPU temperature publishing (every 10 seconds)
  if (mqttManager.shouldPublishTemperature(currentTime)) {
    float cpuTemp = readCPUTemperature();
    mqttManager.publishCpuTemperature(cpuTemp);
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

  delay(1000); // Main loop delay
}

