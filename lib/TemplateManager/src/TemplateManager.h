#ifndef TEMPLATEMANAGER_H
#define TEMPLATEMANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Config.h"

class TemplateManager {
public:
    // Template loading and processing
    static String loadHTMLTemplate(const char* filename);
    static String loadTemplate(const char* templatePath);
    
    // Template downloading and updating
    static bool downloadTemplate();
    static bool downloadFileFromGitHub(const String& filePath, const String& localPath);
    static void checkForTemplateUpdate();
    static void forceTemplateUpdate();
    static void ensureTemplateExists();
    
    // GitHub API functions
    static String makeGitHubAPICall(const String& endpoint);
    static void updateStoredCommitHash();
    
private:
    static Config& _config;
    static Preferences _preferences;
};

#endif // TEMPLATEMANAGER_H
