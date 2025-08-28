#ifndef WIFIHELPER_H
#define WIFIHELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Config.h"

class WiFiHelper {
public:
    // Core WiFi management
    static void setup();
    static void checkConnection();
    
    // Access Point mode
    static bool startAccessPoint();
    static bool isAccessPointMode();
    static String getAccessPointName();
    
    // Recovery mode WiFi setup
    static void setupRecoveryWiFi();
    
    // Web interface handlers
    static void handleConfig(WebServer& server);
    static void handleUpdate(WebServer& server);
    static void handleNetworkScan(WebServer& server);
    static void handleWiFiModeToggle(WebServer& server);
    
    // Template processing
    static String processWiFiTemplateVariables(String html);
    static String getWiFiStatusInfo();
    
    // Utility functions
    static bool isConnected();
    static String getConnectionInfo();
    static void printStatus();
    static String getDebugInfo(); // For debug page HTML
    static String getLocalIP();
    static int getRSSI();
    static String getWiFiStatus();
    static String getSSID();
    static String getGatewayIP();
    static String getDNSIP();
    static String getMACAddress();
    static bool isWiFiConnected();

private:
    static String loadTemplate(const char* templatePath);
    static bool accessPointMode;
};

#endif // WIFIHELPER_H
