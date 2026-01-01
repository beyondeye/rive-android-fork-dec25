#pragma once

/**
 * Platform Detection Header for mprive (Kotlin Multiplatform Rive)
 * 
 * This header provides standardized platform detection macros used throughout
 * the native codebase. It detects the target platform and defines appropriate
 * macros for conditional compilation.
 * 
 * Usage:
 *   #include "platform.hpp"
 *   
 *   #if RIVE_PLATFORM_ANDROID
 *       // Android-specific code
 *   #elif RIVE_PLATFORM_IOS
 *       // iOS-specific code
 *   #elif RIVE_PLATFORM_DESKTOP
 *       // Desktop-specific code
 *   #endif
 */

// =============================================================================
// Platform Detection
// =============================================================================

#if defined(__ANDROID__)
    // Android platform
    #define RIVE_PLATFORM_ANDROID 1
    #define RIVE_PLATFORM_NAME "Android"
    
#elif defined(__APPLE__)
    // Apple platforms (iOS, macOS, tvOS, watchOS)
    #include <TargetConditionals.h>
    
    #if TARGET_OS_IOS
        // iOS (including iPhone, iPad)
        #define RIVE_PLATFORM_IOS 1
        #define RIVE_PLATFORM_NAME "iOS"
        
    #elif TARGET_OS_OSX
        // macOS desktop
        #define RIVE_PLATFORM_MACOS 1
        #define RIVE_PLATFORM_NAME "macOS"
        
    #elif TARGET_OS_TV
        // tvOS (Apple TV)
        #define RIVE_PLATFORM_TVOS 1
        #define RIVE_PLATFORM_NAME "tvOS"
        
    #elif TARGET_OS_WATCH
        // watchOS (Apple Watch)
        #define RIVE_PLATFORM_WATCHOS 1
        #define RIVE_PLATFORM_NAME "watchOS"
        
    #else
        #error "Unknown Apple platform"
    #endif
    
#elif defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
    // WebAssembly via Emscripten
    #define RIVE_PLATFORM_WASM 1
    #define RIVE_PLATFORM_NAME "WebAssembly"
    
#elif defined(__linux__) || defined(__unix__)
    // Linux desktop
    #define RIVE_PLATFORM_LINUX 1
    #define RIVE_PLATFORM_NAME "Linux"
    
#elif defined(_WIN32) || defined(_WIN64)
    // Windows desktop
    #define RIVE_PLATFORM_WINDOWS 1
    #define RIVE_PLATFORM_NAME "Windows"
    
#else
    #error "Unsupported platform - please add support in platform.hpp"
#endif

// =============================================================================
// Platform Categories
// =============================================================================

// Mobile platforms (Android, iOS, tvOS, watchOS)
#if defined(RIVE_PLATFORM_ANDROID) || defined(RIVE_PLATFORM_IOS) || \
    defined(RIVE_PLATFORM_TVOS) || defined(RIVE_PLATFORM_WATCHOS)
    #define RIVE_PLATFORM_MOBILE 1
#else
    #define RIVE_PLATFORM_MOBILE 0
#endif

// Desktop platforms (Linux, macOS, Windows)
#if defined(RIVE_PLATFORM_LINUX) || defined(RIVE_PLATFORM_MACOS) || \
    defined(RIVE_PLATFORM_WINDOWS)
    #define RIVE_PLATFORM_DESKTOP 1
#else
    #define RIVE_PLATFORM_DESKTOP 0
#endif

// Web platforms (WebAssembly)
#if defined(RIVE_PLATFORM_WASM)
    #define RIVE_PLATFORM_WEB 1
#else
    #define RIVE_PLATFORM_WEB 0
#endif

// =============================================================================
// Technology Categories
// =============================================================================

// JNI-based platforms (Android, Desktop JVM, iOS/macOS via Kotlin/Native)
// wasmJS does NOT use JNI (uses JavaScript interop instead)
#if !defined(RIVE_PLATFORM_WASM)
    #define RIVE_PLATFORM_JNI 1
#else
    #define RIVE_PLATFORM_JNI 0
#endif

// Non-JNI platforms
#if defined(RIVE_PLATFORM_WASM)
    #define RIVE_PLATFORM_NO_JNI 1
#else
    #define RIVE_PLATFORM_NO_JNI 0
#endif

// =============================================================================
// Platform-Specific Defaults
// =============================================================================

// Ensure all platform macros are defined (set to 0 if not already defined)
#ifndef RIVE_PLATFORM_ANDROID
    #define RIVE_PLATFORM_ANDROID 0
#endif

#ifndef RIVE_PLATFORM_IOS
    #define RIVE_PLATFORM_IOS 0
#endif

#ifndef RIVE_PLATFORM_MACOS
    #define RIVE_PLATFORM_MACOS 0
#endif

#ifndef RIVE_PLATFORM_TVOS
    #define RIVE_PLATFORM_TVOS 0
#endif

#ifndef RIVE_PLATFORM_WATCHOS
    #define RIVE_PLATFORM_WATCHOS 0
#endif

#ifndef RIVE_PLATFORM_LINUX
    #define RIVE_PLATFORM_LINUX 0
#endif

#ifndef RIVE_PLATFORM_WINDOWS
    #define RIVE_PLATFORM_WINDOWS 0
#endif

#ifndef RIVE_PLATFORM_WASM
    #define RIVE_PLATFORM_WASM 0
#endif

// =============================================================================
// Compile-Time Assertions
// =============================================================================

// Ensure exactly one primary platform is defined
static_assert(
    RIVE_PLATFORM_ANDROID + RIVE_PLATFORM_IOS + RIVE_PLATFORM_MACOS +
    RIVE_PLATFORM_TVOS + RIVE_PLATFORM_WATCHOS + RIVE_PLATFORM_LINUX +
    RIVE_PLATFORM_WINDOWS + RIVE_PLATFORM_WASM == 1,
    "Exactly one primary platform must be defined"
);

// Ensure category macros are consistent
static_assert(
    RIVE_PLATFORM_MOBILE + RIVE_PLATFORM_DESKTOP + RIVE_PLATFORM_WEB >= 1,
    "At least one platform category must be defined"
);

// =============================================================================
// Platform Information Functions
// =============================================================================

namespace rive_mp {
    /**
     * Get the current platform name as a string.
     * @return Platform name (e.g., "Android", "iOS", "Linux")
     */
    inline const char* GetPlatformName() {
        return RIVE_PLATFORM_NAME;
    }
    
    /**
     * Check if running on a mobile platform.
     * @return true if mobile (Android, iOS, tvOS, watchOS)
     */
    inline constexpr bool IsMobilePlatform() {
        return RIVE_PLATFORM_MOBILE == 1;
    }
    
    /**
     * Check if running on a desktop platform.
     * @return true if desktop (Linux, macOS, Windows)
     */
    inline constexpr bool IsDesktopPlatform() {
        return RIVE_PLATFORM_DESKTOP == 1;
    }
    
    /**
     * Check if running on a web platform.
     * @return true if web (WebAssembly)
     */
    inline constexpr bool IsWebPlatform() {
        return RIVE_PLATFORM_WEB == 1;
    }
    
    /**
     * Check if platform uses JNI.
     * @return true if JNI-based platform
     */
    inline constexpr bool UsesJNI() {
        return RIVE_PLATFORM_JNI == 1;
    }
}
