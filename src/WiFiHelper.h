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
    
    // Web interface handlers
    static void handleConfig(WebServer& server);
    static void handleUpdate(WebServer& server);
    static void handleNetworkScan(WebServer& server);
    
    // Utility functions
    static bool isConnected();
    static String getConnectionInfo();
    static void printStatus();
    static String getDebugInfo(); // For debug page HTML

private:
    static String loadTemplate(const char* templatePath);
};

#endif // WIFIHELPER_H
