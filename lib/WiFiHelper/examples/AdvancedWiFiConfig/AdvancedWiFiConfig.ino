/**
 * @file AdvancedWiFiConfig.ino
 * @brief Advanced example with persistent configuration and web interface
 * @author WiFiHelper Library
 * @version 1.0.0
 * 
 * This example demonstrates advanced WiFiHelper features including:
 * - Persistent WiFi configuration storage
 * - Complete web-based configuration interface
 * - Network scanning and selection
 * - WiFi mode switching (Station ↔ Access Point)
 * - Template-based web pages
 * - Recovery mode support
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - At least 4MB flash for LittleFS storage
 * 
 * Features demonstrated:
 * - Integration with Preferences for persistent storage
 * - Custom configuration class compatible with WiFiHelper
 * - Advanced web interface with AJAX network scanning
 * - Template processing and variable replacement
 * - Automatic recovery from connection failures
 */

#include <WiFiHelper.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Simple configuration class for demonstration
class SimpleConfig {
public:
    String wifi_ssid;
    String wifi_password;
    String client_id;
    bool force_ap_mode;
    
    void begin() {
        prefs.begin("wifi-config", false);
        loadFromPreferences();
    }
    
    void loadFromPreferences() {
        wifi_ssid = prefs.getString("ssid", "YourNetwork");
        wifi_password = prefs.getString("password", "YourPassword");
        client_id = prefs.getString("client_id", "ESP32_Advanced");
        force_ap_mode = prefs.getBool("force_ap", false);
    }
    
    void saveWiFiCredentials(const String& ssid, const String& password) {
        wifi_ssid = ssid;
        wifi_password = password;
        prefs.putString("ssid", ssid);
        prefs.putString("password", password);
        Serial.printf("Saved WiFi credentials: %s\n", ssid.c_str());
    }
    
    void saveApMode(bool apMode) {
        force_ap_mode = apMode;
        prefs.putBool("force_ap", apMode);
        Serial.printf("Saved AP mode: %s\n", apMode ? "true" : "false");
    }
    
    const char* getWiFiSSID() const { return wifi_ssid.c_str(); }
    const char* getWiFiPassword() const { return wifi_password.c_str(); }
    const char* getClientId() const { return client_id.c_str(); }
    
private:
    Preferences prefs;
};

// Global configuration instance
SimpleConfig config;

// Make config available to WiFiHelper (optional integration)
// Note: In a real implementation, you would modify WiFiHelper to use this
SimpleConfig* g_config = &config;

// Web server
WebServer server(80);

// Function declarations
void setupWebServer();
void setupTemplates();
void handleRoot();
void handleConfig();
void handleAdvancedStatus();
void handleFactoryReset();
String loadTemplate(const String& templateName);
String processTemplate(String html);

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WiFiHelper Advanced Example ===");
    
    // Initialize filesystem
    if (!LittleFS.begin(true)) {
        Serial.println("ERROR: Failed to mount LittleFS");
        return;
    }
    Serial.println("✓ LittleFS mounted");
    
    // Initialize configuration
    config.begin();
    Serial.printf("✓ Configuration loaded - Client ID: %s\n", config.getClientId());
    
    // Setup basic templates
    setupTemplates();
    
    // Initialize WiFi with configuration
    Serial.println("Setting up WiFi with saved configuration...");
    
    // Note: For this example, WiFiHelper will use its default constants
    // In a real implementation, you would modify WiFiHelper to use the config
    WiFiHelper::setup();
    
    if (WiFiHelper::isConnected()) {
        Serial.println("✓ WiFi setup successful");
        Serial.println(WiFiHelper::getConnectionInfo());
    } else {
        Serial.println("✗ WiFi setup failed - check configuration");
    }
    
    // Setup web server
    setupWebServer();
    
    Serial.println("\n=== Advanced Example Ready ===");
    Serial.println("Web Interface: http://" + WiFiHelper::getLocalIP());
    Serial.println("Features available:");
    Serial.println("  - WiFi configuration with network scanning");
    Serial.println("  - Mode switching (Station ↔ Access Point)"); 
    Serial.println("  - Persistent settings storage");
    Serial.println("  - Advanced status monitoring");
    Serial.println();
}

void loop() {
    static unsigned long lastWiFiCheck = 0;
    static unsigned long lastStatusPrint = 0;
    
    // Handle web requests
    server.handleClient();
    
    // Monitor WiFi connection every 30 seconds
    if (millis() - lastWiFiCheck > 30000) {
        WiFiHelper::checkConnection();
        lastWiFiCheck = millis();
    }
    
    // Print status every 60 seconds
    if (millis() - lastStatusPrint > 60000) {
        Serial.println("=== Status Update ===");
        Serial.println(WiFiHelper::getConnectionInfo());
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        lastStatusPrint = millis();
    }
    
    delay(50);
}

void setupWebServer() {
    /**
     * @brief Configure comprehensive web interface
     */
    
    // Main pages
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/status", handleAdvancedStatus);
    
    // WiFi management (using WiFiHelper)
    server.on("/wifi", []() {
        WiFiHelper::handleConfig(server);
    });
    
    server.on("/wifi-update", HTTP_POST, []() {
        // Custom handler that also updates our config
        if (server.hasArg("ssid") && server.hasArg("password")) {
            config.saveWiFiCredentials(server.arg("ssid"), server.arg("password"));
        }
        WiFiHelper::handleUpdate(server);
    });
    
    server.on("/scan-networks", []() {
        WiFiHelper::handleNetworkScan(server);
    });
    
    server.on("/wifi-mode", HTTP_POST, []() {
        // Custom handler that also updates our config
        if (server.hasArg("mode")) {
            bool apMode = (server.arg("mode") == "ap");
            config.saveApMode(apMode);
        }
        WiFiHelper::handleWiFiModeToggle(server);
    });
    
    // Advanced features
    server.on("/factory-reset", HTTP_POST, handleFactoryReset);
    
    server.on("/api/status", []() {
        // JSON API endpoint for status
        DynamicJsonDocument doc(1024);
        doc["wifi_connected"] = WiFiHelper::isWiFiConnected();
        doc["ap_mode"] = WiFiHelper::isAccessPointMode();
        doc["ssid"] = WiFiHelper::getSSID();
        doc["ip"] = WiFiHelper::getLocalIP();
        doc["rssi"] = WiFiHelper::getRSSI();
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
    
    // Start server
    server.begin();
    Serial.println("✓ Advanced web server started");
}

void setupTemplates() {
    /**
     * @brief Create basic HTML templates in LittleFS
     */
    
    // Create templates directory
    if (!LittleFS.exists("/templates")) {
        LittleFS.mkdir("/templates");
    }
    
    // Simple response template
    String simpleTemplate = R"(
<!DOCTYPE html>
<html>
<head>
    <title>{{TITLE}}</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .button { display: inline-block; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; margin: 10px 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>{{HEADER}}</h1>
        <p>{{MESSAGE}}</p>
        {{EXTRA_CONTENT}}
        <a href="/" class="button">← Back to Main</a>
    </div>
</body>
</html>
)";
    
    File file = LittleFS.open("/templates/simple_response.html", "w");
    if (file) {
        file.print(simpleTemplate);
        file.close();
    }
    
    Serial.println("✓ Basic templates created");
}

void handleRoot() {
    /**
     * @brief Serve advanced main page with live status updates
     */
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Advanced WiFi Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status { padding: 15px; margin: 20px 0; border-radius: 5px; }
        .connected { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }
        .ap-mode { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }
        .disconnected { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }
        .button { display: inline-block; padding: 12px 24px; margin: 10px 5px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; transition: background 0.3s; }
        .button:hover { background: #0056b3; }
        .button.danger { background: #dc3545; }
        .button.danger:hover { background: #c82333; }
        .info-grid { display: grid; grid-template-columns: 1fr 2fr; gap: 15px; margin: 20px 0; }
        .info-label { font-weight: bold; color: #495057; }
        .live-status { border: 1px solid #dee2e6; padding: 15px; border-radius: 5px; background: #f8f9fa; }
        #liveData { font-family: monospace; }
    </style>
    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('liveData').innerHTML = 
                        'Status: ' + (data.wifi_connected ? 'Connected' : (data.ap_mode ? 'Access Point' : 'Disconnected')) + '<br>' +
                        'Network: ' + data.ssid + '<br>' +
                        'IP: ' + data.ip + '<br>' +
                        'Signal: ' + data.rssi + ' dBm<br>' +
                        'Uptime: ' + data.uptime + ' seconds<br>' +
                        'Free Heap: ' + data.free_heap + ' bytes';
                })
                .catch(error => {
                    document.getElementById('liveData').innerHTML = 'Error updating status';
                });
        }
        
        setInterval(updateStatus, 5000); // Update every 5 seconds
        window.onload = updateStatus;
    </script>
</head>
<body>
    <div class="container">
        <h1>🚀 ESP32 Advanced WiFi Configuration</h1>
        
        <div class="status )" + String(WiFiHelper::isWiFiConnected() ? "connected" : (WiFiHelper::isAccessPointMode() ? "ap-mode" : "disconnected")) + R"(">
            <strong>Current Status:</strong> )" + WiFiHelper::getWiFiStatus() + R"(
        </div>
        
        <h3>📊 Live Status</h3>
        <div class="live-status">
            <div id="liveData">Loading...</div>
        </div>
        
        <h3>📋 Configuration</h3>
        <div class="info-grid">
            <div class="info-label">Client ID:</div>
            <div>)" + String(config.getClientId()) + R"(</div>
            
            <div class="info-label">WiFi SSID:</div>
            <div>)" + String(config.getWiFiSSID()) + R"(</div>
            
            <div class="info-label">Current Mode:</div>
            <div>)" + String(config.force_ap_mode ? "Forced Access Point" : "Station (with AP fallback)") + R"(</div>
            
            <div class="info-label">MAC Address:</div>
            <div>)" + WiFiHelper::getMACAddress() + R"(</div>
        </div>
        
        <h3>⚙️ Management</h3>
        <a href="/wifi" class="button">📶 WiFi Settings</a>
        <a href="/status" class="button">📊 Detailed Status</a>
        <a href="/config" class="button">🔧 Advanced Config</a>
        
        <h3>🛠️ System Actions</h3>
        <a href="#" onclick="if(confirm('Reset all settings?')) { fetch('/factory-reset', {method: 'POST'}); }" class="button danger">🏭 Factory Reset</a>
        <a href="#" onclick="if(confirm('Reboot device?')) { window.location='/reboot'; }" class="button">🔄 Reboot</a>
        
        <hr>
        <p><small>
            WiFiHelper Library v1.0.0 | 
            ESP32 Chip: )" + String(ESP.getChipModel()) + R"( | 
            Flash: )" + String(ESP.getFlashChipSize()/1024/1024) + R"( MB
        </small></p>
    </div>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void handleConfig() {
    /**
     * @brief Serve advanced configuration page
     */
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Advanced Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .form-group { margin: 20px 0; }
        .form-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .form-group input, .form-group select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; }
        .button { display: inline-block; padding: 12px 24px; margin: 10px 5px; background: #007bff; color: white; text-decoration: none; border: none; border-radius: 5px; cursor: pointer; }
        .button:hover { background: #0056b3; }
        .back-button { background: #6c757d; }
        .back-button:hover { background: #545b62; }
    </style>
</head>
<body>
    <div class="container">
        <a href="/" class="button back-button">← Back to Main</a>
        
        <h1>🔧 Advanced Configuration</h1>
        
        <form method="post" action="/wifi-update">
            <h3>WiFi Settings</h3>
            
            <div class="form-group">
                <label for="ssid">Network SSID:</label>
                <input type="text" id="ssid" name="ssid" value=")" + String(config.getWiFiSSID()) + R"(" required>
            </div>
            
            <div class="form-group">
                <label for="password">Network Password:</label>
                <input type="password" id="password" name="password" placeholder="Enter new password">
            </div>
            
            <button type="submit" class="button">💾 Save WiFi Settings</button>
        </form>
        
        <form method="post" action="/wifi-mode">
            <h3>Operating Mode</h3>
            
            <div class="form-group">
                <label for="mode">WiFi Mode:</label>
                <select id="mode" name="mode">
                    <option value="station" )" + String(!config.force_ap_mode ? "selected" : "") + R"(>Station Mode (Connect to WiFi)</option>
                    <option value="ap" )" + String(config.force_ap_mode ? "selected" : "") + R"(>Access Point Mode (Create Hotspot)</option>
                </select>
            </div>
            
            <button type="submit" class="button">🔄 Change Mode</button>
        </form>
        
        <h3>📋 Current Settings</h3>
        <p><strong>Client ID:</strong> )" + String(config.getClientId()) + R"(</p>
        <p><strong>Current SSID:</strong> )" + String(config.getWiFiSSID()) + R"(</p>
        <p><strong>Force AP Mode:</strong> )" + String(config.force_ap_mode ? "Yes" : "No") + R"(</p>
        
        <p><small><em>Note: Changes require a restart to take effect.</em></small></p>
    </div>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void handleAdvancedStatus() {
    /**
     * @brief Serve comprehensive status page with all available information
     */
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Advanced Status - ESP32</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 900px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .back-button { display: inline-block; padding: 10px 20px; background: #6c757d; color: white; text-decoration: none; border-radius: 5px; margin-bottom: 20px; }
        .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }
        .status-card { background: #f8f9fa; padding: 20px; border-radius: 5px; border-left: 4px solid #007bff; }
        .debug-info { background: #e9ecef; padding: 15px; border-radius: 5px; margin: 15px 0; }
        pre { background: #212529; color: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; }
        .refresh-button { background: #28a745; color: white; padding: 8px 16px; border: none; border-radius: 3px; cursor: pointer; }
    </style>
    <script>
        function refreshPage() {
            location.reload();
        }
    </script>
</head>
<body>
    <div class="container">
        <a href="/" class="back-button">← Back to Main</a>
        <button onclick="refreshPage()" class="refresh-button" style="float: right;">🔄 Refresh</button>
        
        <h1>📊 Advanced System Status</h1>
        
        <div class="status-grid">
            <div class="status-card">
                <h3>🌐 Network Status</h3>
                <p><strong>Mode:</strong> )" + String(WiFiHelper::isAccessPointMode() ? "Access Point" : "WiFi Station") + R"(</p>
                <p><strong>Status:</strong> )" + WiFiHelper::getWiFiStatus() + R"(</p>
                <p><strong>SSID:</strong> )" + WiFiHelper::getSSID() + R"(</p>
                <p><strong>IP Address:</strong> )" + WiFiHelper::getLocalIP() + R"(</p>
                <p><strong>Signal:</strong> )" + String(WiFiHelper::getRSSI()) + " dBm</p>
            </div>
            
            <div class="status-card">
                <h3>💾 System Info</h3>
                <p><strong>Chip:</strong> )" + String(ESP.getChipModel()) + R"(</p>
                <p><strong>CPU:</strong> )" + String(ESP.getCpuFreqMHz()) + " MHz</p>
                <p><strong>Flash:</strong> )" + String(ESP.getFlashChipSize()/1024/1024) + " MB</p>
                <p><strong>Free Heap:</strong> )" + String(ESP.getFreeHeap()) + " bytes</p>
                <p><strong>Uptime:</strong> )" + String(millis()/1000) + " seconds</p>
            </div>
        </div>
        
        <h3>🔧 WiFiHelper Debug Info</h3>
        <div class="debug-info">
)" + WiFiHelper::getDebugInfo() + R"(
        </div>
        
        <h3>📋 Detailed Status</h3>
        <pre>)" + WiFiHelper::getWiFiStatusInfo() + R"(</pre>
        
        <h3>🔗 Connection Details</h3>
        <pre>)" + WiFiHelper::getConnectionInfo() + R"(</pre>
        
        <h3>⚙️ Configuration</h3>
        <pre>
Client ID: )" + String(config.getClientId()) + R"(
Configured SSID: )" + String(config.getWiFiSSID()) + R"(
Force AP Mode: )" + String(config.force_ap_mode ? "Yes" : "No") + R"(
Library Version: WiFiHelper v1.0.0
        </pre>
    </div>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void handleFactoryReset() {
    /**
     * @brief Reset all configuration to defaults
     */
    // Clear all preferences
    Preferences prefs;
    prefs.begin("wifi-config", false);
    prefs.clear();
    prefs.end();
    
    String html = loadTemplate("simple_response.html");
    html.replace("{{TITLE}}", "Factory Reset");
    html.replace("{{HEADER}}", "Factory Reset Complete");
    html.replace("{{MESSAGE}}", "All settings have been reset to defaults. The device will restart in 5 seconds.");
    html.replace("{{EXTRA_CONTENT}}", "<script>setTimeout(function(){ window.location='/'; }, 5000);</script>");
    
    server.send(200, "text/html", html);
    
    delay(1000);
    ESP.restart();
}

String loadTemplate(const String& templateName) {
    /**
     * @brief Load HTML template from LittleFS
     */
    String path = "/templates/" + templateName;
    if (!LittleFS.exists(path)) {
        return "Template not found: " + templateName;
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        return "Could not open template: " + templateName;
    }
    
    String content = file.readString();
    file.close();
    return content;
}

String processTemplate(String html) {
    /**
     * @brief Process template variables using WiFiHelper and config
     */
    html = WiFiHelper::processWiFiTemplateVariables(html);
    html.replace("{{CLIENT_ID}}", config.getClientId());
    return html;
}
