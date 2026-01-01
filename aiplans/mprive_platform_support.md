# mprive Platform Support Documentation

**Project**: rive-android-fork-dec25  
**Module**: mprive (Kotlin Multiplatform)  
**Last Updated**: January 1, 2026

---

## Table of Contents

1. [Overview](#overview)
2. [Platform Detection](#platform-detection)
3. [Logging Implementation](#logging-implementation)
4. [Platform-Specific Requirements](#platform-specific-requirements)
5. [Future Platform Support](#future-platform-support)

---

## Overview

The mprive module is designed to support multiple platforms through Kotlin Multiplatform. This document describes how platform detection works and what's required for each platform.

### Supported Platforms (Current)

| Platform | Status | Native Code | Rendering Backend |
|----------|--------|-------------|-------------------|
| **Android** | ✅ In Progress | C++ via JNI | PLS Renderer + OpenGL ES |
| **Desktop (Linux)** | ✅ In Progress | C++ via JNI | Skia Renderer |

### Planned Platforms (Future)

| Platform | Status | Native Code | Rendering Backend |
|----------|--------|-------------|-------------------|
| **iOS** | 🔜 Planned | C++ via Kotlin/Native | PLS Renderer + Metal |
| **macOS Desktop** | 🔜 Planned | C++ via Kotlin/Native | Skia Renderer or PLS + Metal |
| **wasmJS** | 🔜 Planned | Kotlin/JS or Emscripten | PLS + WebGL or SkiaWasm |
| **Windows Desktop** | 🔜 Planned | C++ via JNI | Skia Renderer or PLS + D3D12 |

---

## Platform Detection

### Platform Detection Header

**File**: `mprive/src/nativeInterop/cpp/include/platform.hpp`

This header provides standardized platform detection macros used throughout the native codebase.

#### Platform Macros

```cpp
// Specific platform
RIVE_PLATFORM_ANDROID    // Android devices
RIVE_PLATFORM_IOS        // iOS devices
RIVE_PLATFORM_MACOS      // macOS desktop
RIVE_PLATFORM_LINUX      // Linux desktop
RIVE_PLATFORM_WINDOWS    // Windows desktop
RIVE_PLATFORM_WASM       // WebAssembly (wasmJS)

// Platform categories
RIVE_PLATFORM_MOBILE     // Android or iOS
RIVE_PLATFORM_DESKTOP    // Linux, macOS, or Windows
RIVE_PLATFORM_WEB        // wasmJS

// Technology categories
RIVE_PLATFORM_JNI        // Uses JNI (Android, Desktop JVM, Kotlin/Native)
RIVE_PLATFORM_NO_JNI     // No JNI (wasmJS)
```

#### Usage Example

```cpp
#include "platform.hpp"

#if RIVE_PLATFORM_ANDROID
    // Android-specific code
#elif RIVE_PLATFORM_IOS
    // iOS-specific code
#elif RIVE_PLATFORM_DESKTOP
    // Desktop-specific code
#endif
```

### Preprocessor Defines by Platform

| Platform | Standard Defines |
|----------|------------------|
| Android | `__ANDROID__` |
| iOS | `__APPLE__` + `TARGET_OS_IOS` |
| macOS | `__APPLE__` + `TARGET_OS_OSX` |
| Linux | `__linux__` or `__unix__` |
| Windows | `_WIN32` or `_WIN64` |
| wasmJS | `__EMSCRIPTEN__` or `EMSCRIPTEN` |

---

## Logging Implementation

### Overview

Logging in mprive is implemented through a platform-agnostic interface that routes to platform-specific logging systems.

### Platform-Specific Logging Mechanisms

#### Android

**Mechanism**: Android logcat via `__android_log_print()`

**Header**: `<android/log.h>`

**Priority Levels**:
- `ANDROID_LOG_DEBUG` - Debug messages
- `ANDROID_LOG_INFO` - Info messages
- `ANDROID_LOG_WARN` - Warning messages
- `ANDROID_LOG_ERROR` - Error messages

**Example**:
```cpp
__android_log_print(ANDROID_LOG_DEBUG, "rive-mp", "Debug message");
```

**Viewing Logs**:
```bash
adb logcat -s "rive-mp:*"
```

#### iOS

**Mechanism**: Apple's unified logging system

**Options**:
1. `NSLog()` - Traditional logging (Objective-C)
2. `os_log()` - Modern unified logging (iOS 10+)

**Header**: 
- `#import <Foundation/Foundation.h>` (for NSLog)
- `#include <os/log.h>` (for os_log)

**Example (NSLog)**:
```objc
NSLog(@"[%s] [DEBUG] %s", tag, message);
```

**Example (os_log)**:
```c
os_log_debug(OS_LOG_DEFAULT, "[%s] %s", tag, message);
```

**Viewing Logs**:
```bash
# Using Console.app on macOS
# Or in Xcode's console output
```

#### Desktop (Linux/macOS/Windows)

**Mechanism**: Standard C I/O streams

**Functions**:
- `printf()` - Standard output
- `fprintf(stderr)` - Standard error

**Example**:
```cpp
printf("[%s] [DEBUG] %s\n", tag, message);
fprintf(stderr, "[%s] [ERROR] %s\n", tag, message);
```

**Viewing Logs**:
```bash
# Logs appear in terminal/console
./your_app
```

#### wasmJS

**Mechanism**: Browser console or Emscripten logging

**Options**:
1. `emscripten_log()` - Emscripten logging API
2. JavaScript `console.log()` via EM_ASM

**Header**: `<emscripten.h>`

**Example (emscripten_log)**:
```cpp
emscripten_log(EM_LOG_CONSOLE, "[%s] [DEBUG] %s", tag, message);
```

**Example (EM_ASM)**:
```cpp
EM_ASM({
    console.log('[' + UTF8ToString($0) + '] [DEBUG] ' + UTF8ToString($1));
}, tag, message);
```

**Viewing Logs**:
```
Open browser developer console (F12)
```

### Log Levels

All platforms support these log levels:

| Level | Function | Description | Error Stream |
|-------|----------|-------------|--------------|
| **Debug** | `RiveLogD()` | Detailed debug information | No |
| **Info** | `RiveLogI()` | General information | No |
| **Warning** | `RiveLogW()` | Warning messages | No |
| **Error** | `RiveLogE()` | Error messages | Yes (Desktop) |

### Implementation Details

#### Two-Layer API Design

mprive provides a **dual-layer logging API** for optimal flexibility and performance:

1. **Macros (Recommended)**: `LOGD()`, `LOGE()`, `LOGI()`, `LOGW()`
   - Compile-time controlled (enabled only in DEBUG or LOG builds)
   - Zero overhead in release builds (completely removed by preprocessor)
   - Same API as kotlin module for familiarity
   - Default tag: `"rive-mp"`

2. **Functions (Advanced)**: `rive_mp::RiveLogD()`, etc.
   - Always available (both debug and release builds)
   - Multiplatform implementation
   - Support custom tags
   - Use for critical production errors only

#### File: `rive_log.hpp`

```cpp
namespace rive_mp {
    void RiveLogD(const char* tag, const char* message);
    void RiveLogE(const char* tag, const char* message);
    void RiveLogI(const char* tag, const char* message);
    void RiveLogW(const char* tag, const char* message);
    void InitializeRiveLog();
}

// Logging Macros (Compile-Time Controlled)
#if defined(DEBUG) || defined(LOG)
    #define LOG_TAG "rive-mp"
    #define LOGE(...) rive_mp::RiveLogE(LOG_TAG, __VA_ARGS__)
    #define LOGW(...) rive_mp::RiveLogW(LOG_TAG, __VA_ARGS__)
    #define LOGD(...) rive_mp::RiveLogD(LOG_TAG, __VA_ARGS__)
    #define LOGI(...) rive_mp::RiveLogI(LOG_TAG, __VA_ARGS__)
#else
    // In release builds, logging macros expand to nothing
    #define LOGE(...)
    #define LOGW(...)
    #define LOGD(...)
    #define LOGI(...)
#endif
```

#### File: `rive_log.cpp`

```cpp
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
    #else // RIVE_PLATFORM_DESKTOP
        printf("[%s] [DEBUG] %s\n", tag, message);
    #endif
}
```

#### Usage Examples

**Standard Logging (Recommended)**:
```cpp
#include "rive_log.hpp"

// Initialize once
rive_mp::InitializeRiveLog();

// Use macros for regular logging (debug builds only)
LOGD("Frame rendered");
LOGI("Animation started");
LOGW("Performance warning");
LOGE("Non-critical error");
```

**Advanced (Custom Tags)**:
```cpp
// Debug builds only
#if defined(DEBUG) || defined(LOG)
    rive_mp::RiveLogD("rive-renderer", "Custom component log");
#endif
```

**Critical Production Errors**:
```cpp
// Always logged, even in release builds
rive_mp::RiveLogE("critical", "Fatal error - cannot recover");
```

#### Special Considerations

1. **Null Safety**: All log functions check for nullptr
2. **Enable/Disable**: Logging can be disabled at runtime (except errors)
3. **Error Priority**: Errors always logged, even if logging disabled
4. **Platform Initialization**: Platform-specific logging initialized in `InitializeRiveLog()`
5. **Compile-Time Optimization**: Macros provide zero overhead in release builds
6. **API Compatibility**: Macro names match kotlin module for easy migration

---

## Platform-Specific Requirements

### Android

**Native Code**:
- Language: C++17 or later
- Build System: CMake (externalNativeBuild)
- JNI Version: JNI_VERSION_1_6 or later

**Required Headers**:
- `<jni.h>` - JNI interface
- `<android/log.h>` - Logging
- `<android/native_window.h>` - Surface management (for rendering)

**Build Configuration**:
```cmake
find_library(log-lib log)
target_link_libraries(mprive-android ${log-lib})
```

**Runtime**:
- Minimum SDK: API 21 (Android 5.0)
- Recommended SDK: API 26+ (Android 8.0+)

### iOS

**Native Code**:
- Language: C++ with Objective-C++ (`.mm` files)
- Build System: Xcode or CMake
- Kotlin/Native interop

**Required Frameworks**:
- Foundation.framework (for NSLog)
- os.framework (for os_log)
- Metal.framework (for PLS renderer)

**Runtime**:
- Minimum iOS: 12.0
- Recommended iOS: 14.0+

### Desktop (Linux/macOS/Windows)

**Native Code**:
- Language: C++17 or later
- Build System: CMake
- JNI Version: JNI_VERSION_1_6 or later

**Required Dependencies**:
- JDK (for JNI headers)
- Skia libraries (for Skia renderer)

**Build Configuration**:
```cmake
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})
```

**Runtime**:
- Java/Kotlin JVM required
- OpenGL 3.3+ or Vulkan (for rendering)

### wasmJS

**Native Code**:
- Language: C++ compiled to WASM or Kotlin/JS
- Build System: Emscripten (if C++) or Kotlin/JS compiler
- No JNI (uses JavaScript interop)

**Required Dependencies**:
- Emscripten SDK (if using C++)
- WebGL 2.0 or SkiaWasm

**Runtime**:
- Modern browser with WebGL 2.0 support
- WebAssembly support

---

## Future Platform Support

### Adding a New Platform Checklist

When adding support for a new platform, follow this checklist:

#### 1. Platform Detection
- [ ] Update `platform.hpp` with new platform defines
- [ ] Add platform-specific include guards
- [ ] Test preprocessor detection

#### 2. Logging
- [ ] Update `rive_log.cpp` with platform-specific logging
- [ ] Test all log levels (Debug, Info, Warning, Error)
- [ ] Verify log output in platform's console/viewer
- [ ] Document platform-specific log viewing tools

#### 3. JNI/Interop (if applicable)
- [ ] Update `jni_refs.cpp` for platform's runtime
- [ ] Update `jni_helpers.cpp` for platform's type conversions
- [ ] Test JNI method calls and callbacks
- [ ] Handle platform-specific threading models

#### 4. Rendering
- [ ] Implement platform-specific renderer bindings
- [ ] Test GPU acceleration
- [ ] Measure performance benchmarks
- [ ] Document rendering backend choice

#### 5. Build System
- [ ] Create platform-specific CMakeLists.txt or build script
- [ ] Configure native library output
- [ ] Test build on CI/CD
- [ ] Document build requirements

#### 6. Testing
- [ ] Unit tests for platform-specific code
- [ ] Integration tests with Kotlin code
- [ ] Performance tests
- [ ] Visual regression tests

#### 7. Documentation
- [ ] Update this document with platform details
- [ ] Add platform to main README
- [ ] Create platform-specific setup guide
- [ ] Document known limitations

### Platform Priority

Recommended order for adding new platforms:

1. **iOS** (High Priority)
   - Large user base
   - Similar to Android (mobile platform)
   - PLS renderer already supports Metal

2. **macOS Desktop** (Medium Priority)
   - Extends desktop support
   - Can reuse Skia renderer from Linux
   - Good for development/testing

3. **Windows Desktop** (Medium Priority)
   - Completes desktop platform coverage
   - Skia renderer or PLS + D3D12

4. **wasmJS** (Lower Priority)
   - Web platform support
   - Different architecture (no JNI)
   - Requires significant changes

---

## Appendix: Code Examples

### Example: Platform-Specific Initialization

```cpp
void InitializePlatform() {
    #if RIVE_PLATFORM_ANDROID
        RiveLogI("rive-mp", "Initializing Android platform");
        // Android-specific initialization
    #elif RIVE_PLATFORM_IOS
        RiveLogI("rive-mp", "Initializing iOS platform");
        // iOS-specific initialization
    #elif RIVE_PLATFORM_DESKTOP
        RiveLogI("rive-mp", "Initializing Desktop platform");
        // Desktop-specific initialization
    #elif RIVE_PLATFORM_WASM
        RiveLogI("rive-mp", "Initializing WebAssembly platform");
        // wasmJS-specific initialization
    #endif
}
```

### Example: Platform-Specific Resource Management

```cpp
void* AllocateRenderBuffer(size_t size) {
    #if RIVE_PLATFORM_MOBILE
        // Mobile platforms: Use smaller buffers
        size_t mobileSize = std::min(size, MAX_MOBILE_BUFFER_SIZE);
        return malloc(mobileSize);
    #elif RIVE_PLATFORM_DESKTOP
        // Desktop platforms: Can use larger buffers
        return malloc(size);
    #elif RIVE_PLATFORM_WASM
        // WASM: Very limited memory
        size_t wasmSize = std::min(size, MAX_WASM_BUFFER_SIZE);
        return malloc(wasmSize);
    #endif
}
```

---

**End of Platform Support Documentation**
