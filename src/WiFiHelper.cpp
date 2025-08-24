#include "WiFiHelper.h"
#include <LittleFS.h>

// Get config instance
extern Config& config;

// Static member initialization
bool WiFiHelper::accessPointMode = false;

void WiFiHelper::setup() {
    Serial.println("Setting up WiFi connection...");
    
    // Check if forced to AP mode
    if (config.wifi.force_ap_mode) {
        Serial.println("Force AP mode enabled - starting Access Point...");
        if (startAccessPoint()) {
            accessPointMode = true;
            Serial.println("✓ Access Point started successfully");
        } else {
            Serial.println("✗ Failed to start Access Point");
        }
        return;
    }
    
    // Normal WiFi station mode connection attempt
    Serial.printf("Connecting to: %s\n", config.getWiFiSSID());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < ConfigConstants::WiFi::MAX_ATTEMPTS) {
        delay(ConfigConstants::WiFi::RETRY_DELAY);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        accessPointMode = false;
    } else {
        Serial.println("\n✗ Failed to connect to WiFi");
        Serial.println("Starting Access Point mode...");
        
        if (startAccessPoint()) {
            accessPointMode = true;
            Serial.println("✓ Access Point started successfully");
        } else {
            Serial.println("✗ Failed to start Access Point");
        }
    }
}

bool WiFiHelper::startAccessPoint() {
    String apName = getAccessPointName();
    
    Serial.printf("Starting Access Point: %s\n", apName.c_str());
    
    // Configure access point
    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(
        apName.c_str(),
        ConfigConstants::WiFi::AP_PASSWORD,
        ConfigConstants::WiFi::AP_CHANNEL,
        ConfigConstants::WiFi::AP_HIDDEN,
        ConfigConstants::WiFi::AP_MAX_CONNECTIONS
    );
    
    if (success) {
        IPAddress apIP = WiFi.softAPIP();
        Serial.printf("✓ Access Point started\n");
        Serial.printf("SSID: %s\n", apName.c_str());
        Serial.printf("Password: %s\n", ConfigConstants::WiFi::AP_PASSWORD);
        Serial.printf("IP address: %s\n", apIP.toString().c_str());
        Serial.printf("Connect to this network and visit: http://%s\n", apIP.toString().c_str());
        return true;
    } else {
        Serial.println("✗ Failed to start Access Point");
        return false;
    }
}

bool WiFiHelper::isAccessPointMode() {
    return accessPointMode;
}

String WiFiHelper::getAccessPointName() {
    // Use client_id as the access point name
    return String(config.getClientId());
}

void WiFiHelper::checkConnection() {
    // Skip connection checks if we're in AP mode
    if (accessPointMode) {
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n⚠ WiFi connection lost. Attempting to reconnect...");
        WiFi.disconnect();
        WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < ConfigConstants::WiFi::RECONNECT_ATTEMPTS) {
            delay(ConfigConstants::WiFi::RETRY_DELAY);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✓ WiFi reconnected!");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
            accessPointMode = false; // Switch back to station mode
        } else {
            Serial.println("\n✗ Failed to reconnect to WiFi");
            // Optionally switch back to AP mode if reconnection fails repeatedly
        }
    }
}

void WiFiHelper::handleConfig(WebServer& server) {
    String html = loadTemplate("wifi_config.html");
    
    // Replace placeholders with actual values
    html.replace("{{CURRENT_SSID}}", WiFi.SSID());
    html.replace("{{SIGNAL_STRENGTH}}", String(WiFi.RSSI()));
    html.replace("{{WIFI_STATUS}}", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    html.replace("{{CLIENT_ID}}", config.client_id);
    html.replace("{{WIFI_MODE}}", config.wifi.force_ap_mode ? "Access Point" : "Station");
    html.replace("{{WIFI_MODE_TOGGLE}}", config.wifi.force_ap_mode ? "station" : "ap");
    html.replace("{{WIFI_MODE_BUTTON}}", config.wifi.force_ap_mode ? "Switch to Station Mode" : "Switch to Access Point Mode");
    
    server.send(200, "text/html", html);
}

void WiFiHelper::handleUpdate(WebServer& server) {
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        server.send(400, "text/plain", "Missing SSID or password");
        return;
    }
    
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // Save new WiFi credentials using config
    config.saveWiFiCredentials(newSSID, newPassword);
    
    String html = loadTemplate("wifi_updated.html");
    html.replace("{{NEW_SSID}}", newSSID);
    
    server.send(200, "text/html", html);
    
    delay(ConfigConstants::Timing::REBOOT_DELAY);
    ESP.restart();
}

void WiFiHelper::handleNetworkScan(WebServer& server) {
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

bool WiFiHelper::isConnected() {
    // In AP mode, we consider it "connected" for functionality purposes
    return WiFi.status() == WL_CONNECTED || accessPointMode;
}

String WiFiHelper::getConnectionInfo() {
    if (accessPointMode) {
        String info = "Access Point: " + getAccessPointName();
        info += " (" + WiFi.softAPIP().toString() + ")";
        info += " Clients: " + String(WiFi.softAPgetStationNum());
        return info;
    } else if (WiFi.status() == WL_CONNECTED) {
        String info = "WiFi: " + WiFi.SSID();
        info += " (" + WiFi.localIP().toString() + ")";
        info += " RSSI: " + String(WiFi.RSSI()) + "dBm";
        return info;
    } else {
        return "WiFi: Disconnected";
    }
}

void WiFiHelper::printStatus() {
    Serial.println(getConnectionInfo());
}

String WiFiHelper::getDebugInfo() {
    String debugInfo = "";
    
    if (accessPointMode) {
        debugInfo += "<div class='debug-item'><span class='debug-label'>Mode:</span><span class='debug-value success' data-id='network-mode'>Access Point</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>AP Name:</span><span class='debug-value' data-id='network-ssid'>" + getAccessPointName() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>AP IP:</span><span class='debug-value' data-id='network-ip'>" + WiFi.softAPIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>Connected Clients:</span><span class='debug-value' data-id='network-clients'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>AP MAC:</span><span class='debug-value' data-id='network-mac'>" + WiFi.softAPmacAddress() + "</span></div>";
    } else {
        debugInfo += "<div class='debug-item'><span class='debug-label'>Mode:</span><span class='debug-value " + String(WiFi.status() == WL_CONNECTED ? "success" : "error") + "' data-id='network-mode'>WiFi Station</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>WiFi Status:</span><span class='debug-value " + String(WiFi.status() == WL_CONNECTED ? "success" : "error") + "' data-id='network-wifi-status'>" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>SSID:</span><span class='debug-value' data-id='network-ssid'>" + WiFi.SSID() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>IP Address:</span><span class='debug-value' data-id='network-ip'>" + WiFi.localIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>Gateway:</span><span class='debug-value' data-id='network-gateway'>" + WiFi.gatewayIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>DNS:</span><span class='debug-value' data-id='network-dns'>" + WiFi.dnsIP().toString() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>MAC Address:</span><span class='debug-value' data-id='network-mac'>" + WiFi.macAddress() + "</span></div>";
        debugInfo += "<div class='debug-item'><span class='debug-label'>Signal Strength:</span><span class='debug-value' data-id='network-rssi'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    }
    
    return debugInfo;
}

String WiFiHelper::loadTemplate(const char* templatePath) {
    String fullPath = "/templates/" + String(templatePath);
    
    if (!LittleFS.exists(fullPath)) {
        return "<!DOCTYPE html><html><body><h1>Error: Template not found</h1><p>Path: " + fullPath + "</p></body></html>";
    }
    
    File file = LittleFS.open(fullPath, "r");
    if (!file) {
        return "<!DOCTYPE html><html><body><h1>Error: Could not open template</h1><p>Path: " + fullPath + "</p></body></html>";
    }
    
    String html = file.readString();
    file.close();
    return html;
}
