/**
 * @file SystemUtils.cpp
 * @brief Implementation of system utilities for ESP32 hardware information
 * @author ESP_Sandbox Project
 * @date 2025
 * @version 1.0.0
 */

#include "SystemUtils.h"

float SystemUtils::readCPUTemperature() {
    /**
     * @brief Read ESP32 internal temperature sensor
     * 
     * Uses the built-in temperatureRead() function to get the internal die temperature.
     * This sensor is not calibrated and should only be used for relative temperature
     * monitoring or thermal protection purposes.
     * 
     * @return Temperature in Celsius (typically ranges from 30°C to 80°C under normal conditions)
     */
    // ESP32 internal temperature sensor
    // Note: This is not very accurate and is mainly for monitoring purposes
    return temperatureRead();
}

String SystemUtils::getBoardType() {
    /**
     * @brief Retrieve board type from compile-time definition
     * 
     * Checks for BOARD_TYPE preprocessor definition and returns its value.
     * This allows different board configurations to be identified at runtime.
     * 
     * @return Board type string or "Unknown" if not defined
     */
#ifdef BOARD_TYPE
    return String(BOARD_TYPE);
#else
    return "Unknown";
#endif
}

String SystemUtils::getSystemInfo() {
    /**
     * @brief Generate comprehensive system information summary
     * 
     * Combines multiple system information sources into a single formatted string
     * suitable for display in web interfaces, serial output, or logging. Includes
     * hardware specs, performance metrics, and current system status.
     * 
     * @return Multi-line formatted string containing:
     *         - Board type and chip model
     *         - CPU cores and frequency
     *         - Flash memory and free heap
     *         - Current CPU temperature
     *         - System uptime since last reset
     */
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
    /**
     * @brief Retrieve detailed ESP32 chip-specific information
     * 
     * Queries the ESP32 system APIs to gather detailed hardware information
     * about the specific chip variant and its capabilities. Useful for
     * debugging hardware-specific issues or optimizing performance.
     * 
     * @return Multi-line string with chip specifications:
     *         - Chip model (ESP32, ESP32-S2, ESP32-C3, etc.)
     *         - Silicon revision number
     *         - Number of available CPU cores
     *         - Current CPU operating frequency
     */
    String info = "";
    info += "Model: " + String(ESP.getChipModel()) + "\n";
    info += "Revision: " + String(ESP.getChipRevision()) + "\n";
    info += "Cores: " + String(ESP.getChipCores()) + "\n";
    info += "CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    return info;
}

String SystemUtils::getMemoryInfo() {
    /**
     * @brief Retrieve comprehensive memory usage statistics
     * 
     * Queries ESP32 memory management APIs to provide detailed information
     * about flash storage and heap memory usage. Essential for monitoring
     * memory consumption and detecting potential memory leaks.
     * 
     * @return Multi-line string with memory statistics:
     *         - Total flash memory size
     *         - Currently available free heap
     *         - Minimum free heap recorded since boot
     *         - Largest contiguous heap block available for allocation
     */
    String info = "";
    info += "Flash Size: " + formatBytes(ESP.getFlashChipSize()) + "\n";
    info += "Free Heap: " + formatBytes(ESP.getFreeHeap()) + "\n";
    info += "Min Free Heap: " + formatBytes(ESP.getMinFreeHeap()) + "\n";
    info += "Max Alloc Heap: " + formatBytes(ESP.getMaxAllocHeap()) + "\n";
    return info;
}

String SystemUtils::formatUptime(unsigned long milliseconds) {
    /**
     * @brief Convert milliseconds to human-readable uptime format
     * 
     * Takes raw millisecond count (typically from millis() function) and converts
     * it to a formatted string showing days, hours, minutes, and seconds. Handles
     * overflow and provides a user-friendly display format.
     * 
     * @param milliseconds Raw millisecond count to convert
     * @return Formatted uptime string (e.g., "2d 14h 30m 45s")
     * 
     * @note Zero values are omitted except for seconds to keep output concise
     * @example Input: 90061000ms → Output: "1d 1h 1m 1s"
     */
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
    /**
     * @brief Convert byte count to human-readable size format
     * 
     * Automatically selects the most appropriate unit (B, KB, MB) based on the
     * input size and formats the result with appropriate decimal precision.
     * Uses binary (1024) rather than decimal (1000) units for accuracy.
     * 
     * @param bytes Number of bytes to format
     * @return Formatted size string with unit suffix
     * 
     * @details Conversion thresholds:
     *          - < 1024 bytes: Display as "X B"
     *          - < 1MB: Display as "X.X KB" 
     *          - >= 1MB: Display as "X.X MB"
     * 
     * @example formatBytes(2048) → "2.0 KB"
     * @example formatBytes(1536000) → "1.5 MB"
     */
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else return String(bytes / (1024.0 * 1024.0), 1) + " MB";
}
