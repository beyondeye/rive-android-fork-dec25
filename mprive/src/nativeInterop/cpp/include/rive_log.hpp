#pragma once

#include <string>

namespace rive_mp {
    // =============================================================================
    // Variadic logging functions (printf-style formatting)
    // =============================================================================
    
    /**
     * Log a debug message with printf-style formatting.
     * 
     * NOTE: For most use cases, use the LOGD() macro instead, which is
     * compiled out in release builds for zero overhead.
     * 
     * @param tag Log tag/category
     * @param format Printf-style format string
     * @param ... Variable arguments for formatting
     */
    void RiveLogD(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * Log an error message with printf-style formatting.
     * 
     * NOTE: For most use cases, use the LOGE() macro instead.
     * Direct function calls can be used for critical errors that should
     * be logged even in release builds.
     * 
     * @param tag Log tag/category
     * @param format Printf-style format string
     * @param ... Variable arguments for formatting
     */
    void RiveLogE(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * Log an info message with printf-style formatting.
     * 
     * NOTE: For most use cases, use the LOGI() macro instead.
     * 
     * @param tag Log tag/category
     * @param format Printf-style format string
     * @param ... Variable arguments for formatting
     */
    void RiveLogI(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    /**
     * Log a warning message with printf-style formatting.
     * 
     * NOTE: For most use cases, use the LOGW() macro instead.
     * 
     * @param tag Log tag/category
     * @param format Printf-style format string
     * @param ... Variable arguments for formatting
     */
    void RiveLogW(const char* tag, const char* format, ...) __attribute__((format(printf, 2, 3)));
    
    
    /**
     * Initialize the Rive logging system.
     * This should be called once during initialization.
     */
    void InitializeRiveLog();
}

// =============================================================================
// Logging Macros (Compile-Time Controlled)
// =============================================================================

/**
 * Logging macros for compile-time optimization.
 * 
 * These macros are ONLY enabled in debug builds (when DEBUG or LOG is defined).
 * In release builds, they expand to nothing, providing zero overhead.
 * 
 * Usage:
 *   LOGD("Debug message");
 *   LOGE("Error occurred");
 *   LOGI("Info message");
 *   LOGW("Warning message");
 * 
 * Default tag: "rive-mp"
 * To use a custom tag, call the underlying functions directly:
 *   rive_mp::RiveLogD("custom-tag", "Message");
 * 
 * Performance:
 *   - Debug builds: Calls underlying rive_mp::RiveLog*() functions
 *   - Release builds: Completely removed by preprocessor (zero overhead)
 */

#if defined(DEBUG) || defined(LOG)
    #define LOG_TAG "rive-mp"
    
    // Printf-style formatted logging macros
    #define LOGE(...) rive_mp::RiveLogE(LOG_TAG, __VA_ARGS__)
    #define LOGW(...) rive_mp::RiveLogW(LOG_TAG, __VA_ARGS__)
    #define LOGD(...) rive_mp::RiveLogD(LOG_TAG, __VA_ARGS__)
    #define LOGI(...) rive_mp::RiveLogI(LOG_TAG, __VA_ARGS__)
    
    // Simple string logging macros (no formatting)
    // Use these for logging plain strings without printf-style formatting
    #define LOGE_STR(msg) rive_mp::RiveLogE(LOG_TAG, "%s", msg)
    #define LOGW_STR(msg) rive_mp::RiveLogW(LOG_TAG, "%s", msg)
    #define LOGD_STR(msg) rive_mp::RiveLogD(LOG_TAG, "%s", msg)
    #define LOGI_STR(msg) rive_mp::RiveLogI(LOG_TAG, "%s", msg)
#else
    // In release builds, logging macros expand to nothing
    #define LOGE(...)
    #define LOGW(...)
    #define LOGD(...)
    #define LOGI(...)
    #define LOGE_STR(msg)
    #define LOGW_STR(msg)
    #define LOGD_STR(msg)
    #define LOGI_STR(msg)
#endif
