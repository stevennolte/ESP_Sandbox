#ifndef ESP_OTA_UPDATER_H
#define ESP_OTA_UPDATER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

class ESPOTAUpdater {
public:
    // Constructor
    ESPOTAUpdater(const char* githubRepo, int currentFirmwareVersion);
    
    // Main functions
    void checkForUpdates();
    void performUpdate(const char* url);
    
    // Board type detection
    void setBoardType(const String& boardType);
    String getBoardType();
    
    // Configuration
    void setUpdateInterval(unsigned long intervalMs);
    bool shouldCheckForUpdates(unsigned long currentTime);
    void updateLastCheckTime(unsigned long currentTime);
    
    // Callbacks for custom behavior
    typedef void (*UpdateAvailableCallback)(int currentVersion, int newVersion, const String& downloadUrl);
    typedef void (*UpdateProgressCallback)(size_t progress, size_t total);
    typedef void (*UpdateCompleteCallback)(bool success, const String& message);
    
    void setUpdateAvailableCallback(UpdateAvailableCallback callback);
    void setUpdateProgressCallback(UpdateProgressCallback callback);
    void setUpdateCompleteCallback(UpdateCompleteCallback callback);
    
    // Auto-update control
    void enableAutoUpdate(bool enabled = true);
    bool isAutoUpdateEnabled();

private:
    const char* _githubRepo;
    int _currentFirmwareVersion;
    String _boardType;
    unsigned long _lastUpdateCheck;
    unsigned long _updateInterval;
    bool _autoUpdateEnabled;
    
    // Callbacks
    UpdateAvailableCallback _updateAvailableCallback;
    UpdateProgressCallback _updateProgressCallback;
    UpdateCompleteCallback _updateCompleteCallback;
    
    // Private helper functions
    int parseVersionFromTag(const String& tagName);
    String findBoardSpecificFirmware(JsonArray assets);
    String findGenericFirmware(JsonArray assets);
    bool downloadAndInstallFirmware(const String& url);
};

#endif // ESP_OTA_UPDATER_H
