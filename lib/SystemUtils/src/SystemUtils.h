#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <Arduino.h>

class SystemUtils {
public:
    // System information
    static float readCPUTemperature();
    static String getBoardType();
    static String getSystemInfo();
    static String getChipInfo();
    static String getMemoryInfo();
    
    // Timing utilities
    static String formatUptime(unsigned long milliseconds);
    static String formatBytes(size_t bytes);
    
private:
    SystemUtils() = default;
};

#endif // SYSTEMUTILS_H
