/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com  
*********/

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



const char* mqtt_user = "steve";     // <-- Set your MQTT username
const char* mqtt_pass = "Doctor*9";     // <-- Set your MQTT password
// --- Configuration ---
const int FIRMWARE_VERSION = 95; // 1.3 becomes 13, 1.4 becomes 14, etc.
const char* GITHUB_REPO = "stevennolte/ESP_Sandbox"; // Your GitHub repository
const unsigned long updateInterval = 5 * 60 * 1000; // Check for updates every 5 minutes
const char* ssid = "SSEI";
const char* password = "Nd14il!la";

// MQTT broker settings (local, e.g., Mosquitto or Home Assistant)
String mqtt_server_ip = "192.168.1.12"; // Default fallback IP
const int mqtt_port = 1883;
String client_id = "ESP_Default"; // Default value, will be loaded from preferences

// MQTT Manager instance
ESPMQTTManager mqttManager(mqtt_user, mqtt_pass, "192.168.1.12", mqtt_port);

// OTA Updater instance
ESPOTAUpdater otaUpdater(GITHUB_REPO, FIRMWARE_VERSION);

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;   
const int powerPin = 21;  

// LED indicator pin
const int ledPin = 2;  // Built-in LED on most ESP32 boards
const int ledChannel = 0;  // PWM channel for LED
const int ledFreq = 5000;  // PWM frequency
const int ledResolution = 8;  // 8-bit resolution (0-255)
int ledBrightness = 128;  // Default brightness (0-255, where 255 is brightest)

// Preferences and WebServer objects
Preferences preferences;
WebServer server(80);  



OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

WiFiClient espClient;

unsigned long lastUpdateCheck = 0;

// Function declarations
float readCPUTemperature();

// --- OTA Update Callback Functions ---
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

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Give the network stack time to stabilize
  delay(2000);
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

void setupWebServer() {
  // Start mDNS with the client_id as hostname
  if (!MDNS.begin(client_id.c_str())) {
    Serial.println("Error starting mDNS");
  } else {
    Serial.println("mDNS responder started");
    // Add service to mDNS
    MDNS.addService("http", "tcp", 80);
  }
  
  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handleSetClientId);
  server.on("/brightness", HTTP_POST, handleBrightness);
  server.on("/reboot", handleReboot);
  server.begin();
  Serial.println("Web server started on http://" + WiFi.localIP().toString());
  Serial.println("mDNS address: http://" + client_id + ".local");
}

void loadClientId() {
  preferences.begin("esp-config", true); // read-only
  client_id = preferences.getString("client_id", "ESP_Default");
  ledBrightness = preferences.getInt("led_brightness", 128); // Default brightness 128
  preferences.end();
  
  // Update MQTT topics with the loaded client_id
  mqttManager.updateTopics(client_id);
  
  Serial.println("Loaded Client ID: " + client_id);
  Serial.println("Loaded LED Brightness: " + String(ledBrightness));
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 IoT Device Starting...");
  Serial.println("Board Type: " + getBoardType());
  Serial.printf("Firmware Version: %d\n", FIRMWARE_VERSION);
  
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, HIGH); // Power on the DS18B20 sensor
  
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");
  
  // Initialize LED PWM channel
  ledcSetup(ledChannel, ledFreq, ledResolution);
  ledcAttachPin(ledPin, ledChannel);
  ledcWrite(ledChannel, 0); // Start with LED off
  
  sensors.begin();

  // Load client ID from preferences
  loadClientId();

  setup_wifi();
  Serial.println("Connected to WiFi");
  
  // Discover Home Assistant server and setup MQTT
  mqtt_server_ip = mqttManager.discoverServer();
  mqttManager.updateServerIP(mqtt_server_ip);
  mqttManager.begin(client_id);
  Serial.println("Using MQTT server: " + mqtt_server_ip);

  // Setup web server
  setupWebServer();

  // Initialize OTA updater callbacks
  otaUpdater.setUpdateAvailableCallback(onUpdateAvailable);
  otaUpdater.setUpdateProgressCallback(onUpdateProgress);
  otaUpdater.setUpdateCompleteCallback(onUpdateComplete);
  otaUpdater.setBoardType(getBoardType());
  otaUpdater.enableAutoUpdate(true);

  // Perform initial update check
  otaUpdater.checkForUpdates();
  lastUpdateCheck = millis();
}

void loop() {
  // Flash LED to indicate loop activity with adjustable brightness
  ledcWrite(ledChannel, ledBrightness);
  delay(50); // LED on for 50ms
  ledcWrite(ledChannel, 0);
  
  // Handle web server requests
  server.handleClient();
  
  // Ensure MQTT connection and handle messages
  if (!mqttManager.isConnected()) {
    mqttManager.connect();
  }
  mqttManager.loop();

  unsigned long currentTime = millis();

  // Publish CPU temperature every 10 seconds
  if (mqttManager.shouldPublishTemperature(currentTime)) {
    float cpuTemp = readCPUTemperature();
    mqttManager.publishCpuTemperature(cpuTemp);
    mqttManager.updateLastPublishTime(currentTime);
  }

  // Publish firmware version every 5 minutes
  if (mqttManager.shouldPublishFirmwareVersion(currentTime)) {
    mqttManager.publishFirmwareVersion(FIRMWARE_VERSION);
    mqttManager.updateLastVersionPublishTime(currentTime);
  }

  // Check for updates every 5 minutes
  if (currentTime - lastUpdateCheck > updateInterval) {
    otaUpdater.checkForUpdates();
    lastUpdateCheck = currentTime;
  }

  // Re-discover MQTT server every 15 minutes
  if (mqttManager.shouldRediscoverServer(currentTime)) {
    Serial.println("Re-discovering MQTT server...");
    String newMQTTServer = mqttManager.discoverServer();
    mqttManager.updateServerIP(newMQTTServer);
    mqtt_server_ip = newMQTTServer;
    mqttManager.updateLastDiscoveryTime(currentTime);
  }

  delay(1000);
}

