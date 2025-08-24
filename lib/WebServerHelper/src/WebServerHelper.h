#ifndef WEBSERVERHELPER_H
#define WEBSERVERHELPER_H

#include <WebServer.h>
#include "Config.h"

class WebServerHelper {
public:
    // Initialize the helper with a reference to the server
    static void init(WebServer* server);
    
    // Basic handlers
    static void handleRoot();
    static void handleSetClientId();
    static void handleBrightness();
    static void handleReboot();
    
    // Debug handlers
    static void handleDebug();
    static void handleDebugData();
    
    // File management handlers
    static void handleFileList();
    static void handleFileDownload();
    static void handleFileUpload();
    static void handleFileUploadComplete();
    
    // Firmware management handlers
    static void handleFirmwareUpload();
    static void handleFirmwareUploadComplete();
    
    // Setup all routes
    static void setupRoutes();
    
private:
    static WebServer* _server;
    static Config& _config;
    
    // Helper functions
    static String loadHTMLTemplate(const char* filename);
};

#endif // WEBSERVERHELPER_H
