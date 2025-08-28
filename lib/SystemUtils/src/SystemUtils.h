/**
 * @file SystemUtils.h
 * @brief System utilities library for ESP32 hardware information and diagnostics
 * @author ESP_Sandbox Project
 * @date 2025
 * @version 1.0.0
 */

#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <Arduino.h>

/**
 * @class SystemUtils
 * @brief Utility class providing system information and hardware diagnostics for ESP32
 * 
 * This class provides static methods to retrieve various system information including
 * CPU temperature, memory usage, chip details, and formatting utilities for uptime
 * and byte sizes. All methods are static, making this a utility class that doesn't
 * require instantiation.
 * 
 * @note This class uses a private constructor to prevent instantiation
 */
class SystemUtils {
public:
    /**
     * @name System Information Functions
     * @brief Functions to retrieve hardware and system information
     * @{
     */
    
    /**
     * @brief Read the internal CPU temperature sensor
     * @return Temperature in Celsius as a float value
     * @note This temperature reading is not very accurate and should be used 
     *       for monitoring purposes only. The sensor is built into the ESP32 chip.
     * @warning Temperature accuracy can vary significantly between different ESP32 chips
     */
    static float readCPUTemperature();
    
    /**
     * @brief Get the board type identifier
     * @return String containing the board type name
     * @note Returns "Unknown" if BOARD_TYPE is not defined in the build configuration
     */
    static String getBoardType();
    
    /**
     * @brief Get comprehensive system information summary
     * @return Multi-line string containing board type, chip info, memory status, 
     *         CPU frequency, temperature, and uptime
     * @details This function combines multiple system information sources into 
     *          a formatted string suitable for display or logging
     */
    static String getSystemInfo();
    
    /**
     * @brief Get detailed chip information
     * @return Multi-line string containing chip model, revision, core count, and CPU frequency
     * @details Provides ESP32-specific hardware information including:
     *          - Chip model (ESP32, ESP32-S2, etc.)
     *          - Silicon revision number
     *          - Number of CPU cores
     *          - Current CPU frequency in MHz
     */
    static String getChipInfo();
    
    /**
     * @brief Get detailed memory usage information
     * @return Multi-line string containing flash size and heap memory statistics
     * @details Provides comprehensive memory information including:
     *          - Total flash memory size
     *          - Current free heap memory
     *          - Minimum free heap since boot
     *          - Maximum allocatable heap block size
     */
    static String getMemoryInfo();
    
    /** @} */ // End of System Information Functions
    
    /**
     * @name Utility Functions
     * @brief Helper functions for formatting and time calculations
     * @{
     */
    
    /**
     * @brief Format milliseconds into human-readable uptime string
     * @param milliseconds Time in milliseconds (typically from millis() function)
     * @return Formatted string in format "Xd Xh Xm Xs" (days, hours, minutes, seconds)
     * @details Converts raw milliseconds into a readable format showing days, hours, 
     *          minutes, and seconds. Zero values are omitted except for seconds.
     * @example formatUptime(90061000) returns "1d 1h 1m 1s"
     */
    static String formatUptime(unsigned long milliseconds);
    
    /**
     * @brief Format byte count into human-readable size string
     * @param bytes Number of bytes to format
     * @return Formatted string with appropriate unit (B, KB, or MB)
     * @details Automatically selects the most appropriate unit:
     *          - Bytes (B) for values < 1024
     *          - Kilobytes (KB) for values < 1024*1024
     *          - Megabytes (MB) for larger values
     * @example formatBytes(2048) returns "2.0 KB"
     */
    static String formatBytes(size_t bytes);
    
    /** @} */ // End of Utility Functions
    
private:
    /**
     * @brief Private constructor to prevent instantiation
     * @note This class is designed to be used as a static utility class only
     */
    SystemUtils() = default;
};

#endif // SYSTEMUTILS_H
