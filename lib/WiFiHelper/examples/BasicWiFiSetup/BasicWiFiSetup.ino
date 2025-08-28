/**
 * @file BasicWiFiSetup.ino
 * @brief Basic example demonstrating WiFiHelper library usage
 * @author WiFiHelper Library
 * @version 1.0.0
 * 
 * This example shows how to use the WiFiHelper library for basic WiFi
 * connectivity with automatic Access Point fallback. Perfect for IoT
 * projects that need reliable network connectivity.
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - Optional: LED on pin 2 for status indication
 * 
 * Features demonstrated:
 * - Automatic WiFi connection with AP fallback
 * - Basic web server with WiFi status page
 * - Network information display
 * - Connection monitoring and recovery
 */

#include <WiFiHelper.h>
#include <WebServer.h>
#include <LittleFS.h>

// Configuration - modify these for your network
const char* WIFI_SSID = "YourNetworkName";
const char* WIFI_PASSWORD = "YourNetworkPassword";

// Optional: LED pin for status indication
const int STATUS_LED_PIN = 2;

// Web server instance
WebServer server(80);

// Function declarations
void setupWebServer();
void handleRoot();
void handleStatus();
void handleWiFiConfig();
void updateStatusLED();

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WiFiHelper Basic Example ===");
    
    // Initialize status LED (optional)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Initialize filesystem for templates (optional)
    if (!LittleFS.begin(true)) {
        Serial.println("Warning: LittleFS mount failed - templates won't work");
    }
    
    // Update default WiFi credentials (in real implementation)
    // For this example, you would modify the constants in WiFiHelper.cpp
    // or use a configuration system
    
    Serial.println("Initializing WiFi...");
    
    // Setup WiFi with automatic AP fallback
    WiFiHelper::setup();
    
    // Check connection status
    if (WiFiHelper::isConnected()) {
        Serial.println("✓ Network ready!");
        Serial.println(WiFiHelper::getConnectionInfo());
        digitalWrite(STATUS_LED_PIN, HIGH); // Success indication
    } else {
        Serial.println("✗ Network setup failed");
        // LED will blink in loop to indicate error
    }
    
    // Setup web server
    setupWebServer();
    
    Serial.println("\n=== Setup Complete ===");
    Serial.println("Available endpoints:");
    Serial.println("  http://" + WiFiHelper::getLocalIP() + "/");
    Serial.println("  http://" + WiFiHelper::getLocalIP() + "/status");
    Serial.println("  http://" + WiFiHelper::getLocalIP() + "/wifi");
    Serial.println();
}

void loop() {
    static unsigned long lastCheck = 0;
    static unsigned long lastStatusUpdate = 0;
    
    // Handle web server requests
    server.handleClient();
    
    // Check WiFi connection every 30 seconds
    if (millis() - lastCheck > 30000) {
        WiFiHelper::checkConnection();
        lastCheck = millis();
        
        // Print status update
        Serial.println("Status: " + WiFiHelper::getConnectionInfo());
    }
    
    // Update status LED every 2 seconds
    if (millis() - lastStatusUpdate > 2000) {
        updateStatusLED();
        lastStatusUpdate = millis();
    }
    
    delay(100); // Small delay to prevent excessive CPU usage
}

void setupWebServer() {
    /**
     * @brief Configure web server endpoints
     * 
     * Sets up basic web interface with WiFi status and configuration pages.
     * Demonstrates integration between WiFiHelper and web server functionality.
     */
    
    // Main page
    server.on("/", handleRoot);
    
    // WiFi status page  
    server.on("/status", handleStatus);
    
    // WiFi configuration (uses WiFiHelper handlers)
    server.on("/wifi", []() {
        WiFiHelper::handleConfig(server);
    });
    
    server.on("/wifi-update", HTTP_POST, []() {
        WiFiHelper::handleUpdate(server);
    });
    
    server.on("/scan", []() {
        WiFiHelper::handleNetworkScan(server);
    });
    
    server.on("/wifi-mode", HTTP_POST, []() {
        WiFiHelper::handleWiFiModeToggle(server);
    });
    
    // Start server
    server.begin();
    Serial.println("✓ Web server started on port 80");
}

void handleRoot() {
    /**
     * @brief Serve main page with basic device information
     */
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WiFiHelper Example</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status { padding: 15px; margin: 20px 0; border-radius: 5px; }
        .connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }
        .disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }
        .ap-mode { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }
        .button { display: inline-block; padding: 10px 20px; margin: 10px 5px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; }
        .button:hover { background: #0056b3; }
        .info-grid { display: grid; grid-template-columns: 1fr 2fr; gap: 10px; margin: 20px 0; }
        .info-label { font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 ESP32 WiFiHelper Example</h1>
        
        <div class="status )" + String(WiFiHelper::isWiFiConnected() ? "connected" : (WiFiHelper::isAccessPointMode() ? "ap-mode" : "disconnected")) + R"(">
            <strong>Status:</strong> )" + WiFiHelper::getWiFiStatus() + R"(
        </div>
        
        <div class="info-grid">
            <div class="info-label">Network Mode:</div>
            <div>)" + String(WiFiHelper::isAccessPointMode() ? "Access Point" : "WiFi Station") + R"(</div>
            
            <div class="info-label">Network Name:</div>
            <div>)" + WiFiHelper::getSSID() + R"(</div>
            
            <div class="info-label">IP Address:</div>
            <div>)" + WiFiHelper::getLocalIP() + R"(</div>
            
            <div class="info-label">MAC Address:</div>
            <div>)" + WiFiHelper::getMACAddress() + R"(</div>
            
            <div class="info-label">Signal Strength:</div>
            <div>)" + String(WiFiHelper::getRSSI()) + " dBm" + R"(</div>
        </div>
        
        <h3>Available Actions</h3>
        <a href="/status" class="button">📊 Detailed Status</a>
        <a href="/wifi" class="button">⚙️ WiFi Configuration</a>
        <a href="/scan" class="button">📡 Scan Networks</a>
        
        <h3>Library Information</h3>
        <p><strong>WiFiHelper Library</strong> - Comprehensive WiFi management for ESP32</p>
        <p>Features: Automatic AP fallback, web configuration, template processing, recovery mode</p>
        
        <hr>
        <p><small>
            Uptime: )" + String(millis()/1000) + R"( seconds | 
            Free Heap: )" + String(ESP.getFreeHeap()) + R"( bytes
        </small></p>
    </div>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void handleStatus() {
    /**
     * @brief Serve detailed WiFi status information
     */
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Status - ESP32</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .debug-info { background: #f8f9fa; padding: 20px; border-radius: 5px; margin: 20px 0; }
        .back-button { display: inline-block; padding: 10px 20px; background: #6c757d; color: white; text-decoration: none; border-radius: 5px; margin-bottom: 20px; }
        pre { background: #e9ecef; padding: 15px; border-radius: 5px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="container">
        <a href="/" class="back-button">← Back to Main</a>
        
        <h1>📊 WiFi Status Details</h1>
        
        <h3>Connection Information</h3>
        <pre>)" + WiFiHelper::getWiFiStatusInfo() + R"(</pre>
        
        <h3>Debug Information</h3>
        <div class="debug-info">
)" + WiFiHelper::getDebugInfo() + R"(
        </div>
        
        <h3>Raw Connection Info</h3>
        <pre>)" + WiFiHelper::getConnectionInfo() + R"(</pre>
        
        <h3>System Information</h3>
        <pre>
Chip Model: )" + String(ESP.getChipModel()) + R"(
CPU Frequency: )" + String(ESP.getCpuFreqMHz()) + R"( MHz
Flash Size: )" + String(ESP.getFlashChipSize()) + R"( bytes
Free Heap: )" + String(ESP.getFreeHeap()) + R"( bytes
Uptime: )" + String(millis()/1000) + R"( seconds
        </pre>
    </div>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void updateStatusLED() {
    /**
     * @brief Update status LED based on WiFi connection state
     * 
     * LED patterns:
     * - Solid ON: WiFi connected
     * - Slow blink: Access Point mode
     * - Fast blink: Disconnected/Error
     */
    static bool ledState = false;
    static int blinkCount = 0;
    
    if (WiFiHelper::isWiFiConnected()) {
        // Solid on for WiFi connected
        digitalWrite(STATUS_LED_PIN, HIGH);
    } else if (WiFiHelper::isAccessPointMode()) {
        // Slow blink for AP mode (every 4 calls = ~8 seconds)
        if (blinkCount % 4 == 0) {
            ledState = !ledState;
            digitalWrite(STATUS_LED_PIN, ledState);
        }
    } else {
        // Fast blink for disconnected (every call = ~2 seconds)
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState);
    }
    
    blinkCount++;
}
