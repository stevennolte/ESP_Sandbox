#include "SystemUtils.h"

float SystemUtils::readCPUTemperature() {
    // ESP32 internal temperature sensor
    // Note: This is not very accurate and is mainly for monitoring purposes
    return temperatureRead();
}

String SystemUtils::getBoardType() {
#ifdef BOARD_TYPE
    return String(BOARD_TYPE);
#else
    return "Unknown";
#endif
}

String SystemUtils::getSystemInfo() {
    String info = "";
    info += "Board: " + getBoardType() + "\n";
    info += "Chip: " + String(ESP.getChipModel()) + "\n";
    info += "Cores: " + String(ESP.getChipCores()) + "\n";
    info += "CPU Freq: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "Flash: " + formatBytes(ESP.getFlashChipSize()) + "\n";
    info += "Free Heap: " + formatBytes(ESP.getFreeHeap()) + "\n";
    info += "CPU Temp: " + String(readCPUTemperature(), 1) + "°C\n";
    info += "Uptime: " + formatUptime(millis()) + "\n";
    return info;
}

String SystemUtils::getChipInfo() {
    String info = "";
    info += "Model: " + String(ESP.getChipModel()) + "\n";
    info += "Revision: " + String(ESP.getChipRevision()) + "\n";
    info += "Cores: " + String(ESP.getChipCores()) + "\n";
    info += "CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    return info;
}

String SystemUtils::getMemoryInfo() {
    String info = "";
    info += "Flash Size: " + formatBytes(ESP.getFlashChipSize()) + "\n";
    info += "Free Heap: " + formatBytes(ESP.getFreeHeap()) + "\n";
    info += "Min Free Heap: " + formatBytes(ESP.getMinFreeHeap()) + "\n";
    info += "Max Alloc Heap: " + formatBytes(ESP.getMaxAllocHeap()) + "\n";
    return info;
}

String SystemUtils::formatUptime(unsigned long milliseconds) {
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    String uptime = "";
    if (days > 0) uptime += String(days) + "d ";
    if (hours % 24 > 0) uptime += String(hours % 24) + "h ";
    if (minutes % 60 > 0) uptime += String(minutes % 60) + "m ";
    uptime += String(seconds % 60) + "s";
    
    return uptime;
}

String SystemUtils::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else return String(bytes / (1024.0 * 1024.0), 1) + " MB";
}
