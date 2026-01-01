#include "rive_log.hpp"
#include "platform.hpp"
#include <cstdio>

// Platform-specific headers
#if RIVE_PLATFORM_ANDROID
    #include <android/log.h>
#elif RIVE_PLATFORM_IOS
    #import <Foundation/Foundation.h>
#elif RIVE_PLATFORM_WASM
    #include <emscripten.h>
#endif

namespace rive_mp {
    // Logging enabled flag
    static bool g_LoggingEnabled = true;

    void InitializeRiveLog() {
        g_LoggingEnabled = true;
        
        #if RIVE_PLATFORM_ANDROID
            __android_log_print(ANDROID_LOG_INFO, "rive-mp", "Rive multiplatform logging initialized (%s)", GetPlatformName());
        #elif RIVE_PLATFORM_IOS
            NSLog(@"[rive-mp] [INFO] Rive multiplatform logging initialized (%s)", GetPlatformName());
        #elif RIVE_PLATFORM_WASM
            emscripten_log(EM_LOG_CONSOLE, "[rive-mp] [INFO] Rive multiplatform logging initialized (%s)", GetPlatformName());
        #else // Desktop platforms (Linux, macOS, Windows)
            printf("[rive-mp] [INFO] Rive multiplatform logging initialized (%s)\n", GetPlatformName());
        #endif
    }

    void RiveLogD(const char* tag, const char* message) {
        if (!g_LoggingEnabled || tag == nullptr || message == nullptr) {
            return;
        }
        
        #if RIVE_PLATFORM_ANDROID
            __android_log_print(ANDROID_LOG_DEBUG, tag, "%s", message);
        #elif RIVE_PLATFORM_IOS
            NSLog(@"[%s] [DEBUG] %s", tag, message);
        #elif RIVE_PLATFORM_WASM
            emscripten_log(EM_LOG_CONSOLE, "[%s] [DEBUG] %s", tag, message);
        #else // Desktop platforms
            printf("[%s] [DEBUG] %s\n", tag, message);
        #endif
    }

    void RiveLogE(const char* tag, const char* message) {
        if (tag == nullptr || message == nullptr) {
            return;
        }
        
        #if RIVE_PLATFORM_ANDROID
            __android_log_print(ANDROID_LOG_ERROR, tag, "%s", message);
        #elif RIVE_PLATFORM_IOS
            NSLog(@"[%s] [ERROR] %s", tag, message);
        #elif RIVE_PLATFORM_WASM
            emscripten_log(EM_LOG_ERROR, "[%s] [ERROR] %s", tag, message);
        #else // Desktop platforms
            fprintf(stderr, "[%s] [ERROR] %s\n", tag, message);
        #endif
    }

    void RiveLogI(const char* tag, const char* message) {
        if (!g_LoggingEnabled || tag == nullptr || message == nullptr) {
            return;
        }
        
        #if RIVE_PLATFORM_ANDROID
            __android_log_print(ANDROID_LOG_INFO, tag, "%s", message);
        #elif RIVE_PLATFORM_IOS
            NSLog(@"[%s] [INFO] %s", tag, message);
        #elif RIVE_PLATFORM_WASM
            emscripten_log(EM_LOG_CONSOLE, "[%s] [INFO] %s", tag, message);
        #else // Desktop platforms
            printf("[%s] [INFO] %s\n", tag, message);
        #endif
    }

    void RiveLogW(const char* tag, const char* message) {
        if (!g_LoggingEnabled || tag == nullptr || message == nullptr) {
            return;
        }
        
        #if RIVE_PLATFORM_ANDROID
            __android_log_print(ANDROID_LOG_WARN, tag, "%s", message);
        #elif RIVE_PLATFORM_IOS
            NSLog(@"[%s] [WARN] %s", tag, message);
        #elif RIVE_PLATFORM_WASM
            emscripten_log(EM_LOG_WARN, "[%s] [WARN] %s", tag, message);
        #else // Desktop platforms
            printf("[%s] [WARN] %s\n", tag, message);
        #endif
    }
}
