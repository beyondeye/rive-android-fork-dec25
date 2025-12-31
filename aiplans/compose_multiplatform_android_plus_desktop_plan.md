# Implementation Plan: Compose Multiplatform Rendering (Android + Desktop)

**Project**: rive-android-fork-dec25  
**Module**: mprive (Kotlin Multiplatform)  
**Target Platforms**: Android, Desktop/Linux JVM  
**Date**: December 31, 2025  
**Status**: Planning Phase  
**Related Documents**: See [mprenderer.md](mprenderer.md) for detailed multiplatform renderer architecture

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Decisions](#architecture-decisions)
3. [Project Structure](#project-structure)
4. [Implementation Phases](#implementation-phases)
5. [Performance Optimization Roadmap](#performance-optimization-roadmap)
6. [Testing Strategy](#testing-strategy)
7. [Success Criteria](#success-criteria)

---

## Overview

### Goal
Add Kotlin Multiplatform support to the rive-android project, enabling Rive animations in Jetpack Compose for both Android and Desktop/Linux platforms.

### Scope
- **Android**: PLS Renderer with TextureView (primary), Canvas/Bitmap (fallback)
- **Desktop**: **Skia Renderer** (GPU-accelerated via Compose Desktop's Skia backend) ⭐
- **API**: Similar to existing kotlin module (package: `app.rive.mp`)
- **Focus**: Compose Multiplatform only (no View-based components)
- **Audio**: Included for both platforms
- **Renderer**: rive_pls_renderer (Android) + rive_skia_renderer (Desktop)

### Key Discovery ✨
Rive runtime provides **two rendering backends**:
1. **PLS Renderer** (`rive_pls_renderer`) - Direct GPU rendering (used on Android)
2. **Skia Renderer** (`rive_skia_renderer`) - Renders to Skia Canvas (perfect for Desktop!)

This discovery enables **GPU-accelerated rendering on Desktop** without complex OpenGL context management, as Compose Desktop already uses Skia.

### Out of Scope (First Release)
- iOS support
- WebAssembly support
- Text run bindings
- Audio playback bindings (stub only)
- View-based components

---

## Architecture Decisions

### 1. Code Organization

**Selected Approach**: Shared source directory (Option B)

```
mprive/
├── src/
│   ├── commonMain/          # Platform-agnostic Kotlin code
│   ├── androidMain/         # Android-specific Kotlin + JNI entry
│   ├── desktopMain/         # Desktop-specific Kotlin + JNI entry
│   └── nativeInterop/       # SHARED C++ CODE
│       └── cpp/
│           ├── include/     # Shared headers
│           └── src/
│               ├── jni_common/   # Common JNI utilities
│               └── bindings/     # Shared JNI bindings
```

**Rationale**: 
- Maximizes code reuse
- Single source of truth for JNI bindings
- Easier to maintain and extend
- Platform differences handled via preprocessor macros

### 2. Rendering Approaches

#### Android: PLS Renderer (Primary)

**PLS Renderer + TextureView** ⭐ (Recommended)
- **Use Case**: Maximum performance, native Android rendering
- **How**: TextureView → SurfaceTexture → PLS Renderer → OpenGL ES → GPU
- **Performance**: 0.5-2ms per frame (zero-copy GPU rendering)
- **Pros**: Maximum performance, zero-copy, production-proven
- **Cons**: Android-specific, requires TextureView management
- **API**: `RiveUI()` composable (similar to kotlin module)

#### Desktop: Skia Renderer (Primary) ✨

**Skia Renderer + Compose Canvas** ⭐ (Recommended)
- **Use Case**: GPU-accelerated rendering on Desktop
- **How**: Rive → Skia Renderer → Compose Desktop's Skia Canvas → Skia GPU Backend → GPU
- **Performance**: 1-3ms per frame (~30% overhead vs PLS, but **300-600% faster than bitmap approach**)
- **Pros**: 
  - GPU-accelerated via Skia's GPU backend (Ganesh/Graphite)
  - Native integration with Compose Desktop (no manual GL context)
  - Simple implementation (just pass Skia canvas to renderer)
  - No CPU-GPU pixel copies
- **Cons**: ~30% slower than PLS direct rendering (but still excellent performance)
- **API**: `RiveUI()` composable (same API across platforms)

**Architecture**:
```
Rive Animation → SkiaRenderer → Skia Canvas → Skia GPU Backend → GPU → Display
                                     ↑
                              (from Compose Desktop)
```

#### Canvas/Bitmap Fallback (Not Recommended)

**Off-screen rendering with pixel copy**
- **Use Case**: Fallback when GPU-accelerated options unavailable
- **Performance**: 5-15ms per frame (300-600% slower than Skia renderer)
- **Why avoid**: Expensive GPU→CPU→GPU copies, significantly slower
- **When to use**: Only if Skia renderer integration fails

### 3. Build Configuration

**Android**: 
- **Native Build**: Automatic via CMake (externalNativeBuild)
- **Output**: libmprive-android.so

**Desktop**:
- **Native Build**: Manual script → Precompiled library
- **Output**: libmprive-desktop.so (checked into source control)
- **Location**: `mprive/src/desktopMain/resources/jnilibs/linux-x86-64/`

---

## Project Structure

### Final Directory Layout

```
mprive/
├── build.gradle.kts
├── src/
│   ├── commonMain/kotlin/app/rive/mp/
│   │   ├── RiveFile.kt              # expect class
│   │   ├── Artboard.kt              # expect class
│   │   ├── Animation.kt             # expect class
│   │   ├── StateMachine.kt          # expect class
│   │   ├── RiveUI.kt                # @Composable - common interface
│   │   ├── RiveCanvas.kt            # @Composable - Canvas-based rendering
│   │   ├── CommandQueue.kt          # expect class
│   │   ├── RenderBuffer.kt          # expect class
│   │   └── core/
│   │       ├── Fit.kt
│   │       ├── Alignment.kt
│   │       └── ...
│   │
│   ├── androidMain/
│   │   ├── AndroidManifest.xml
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── premake5_cpp_runtime.lua
│   │   │   ├── include/
│   │   │   │   └── android_surface_helpers.hpp
│   │   │   └── src/
│   │   │       ├── jni_android.cpp        # Android JNI entry point
│   │   │       └── android_surface.cpp    # TextureView/Surface helpers
│   │   └── kotlin/app/rive/mp/
│   │       ├── RiveFile.android.kt        # actual class
│   │       ├── Artboard.android.kt
│   │       ├── RiveUI.android.kt          # TextureView-based impl
│   │       ├── RiveCanvas.android.kt      # Canvas-based impl
│   │       ├── CommandQueue.android.kt
│   │       └── ...
│   │
│   ├── desktopMain/
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── premake5_desktop_runtime.lua
│   │   │   ├── include/
│   │   │   │   └── desktop_gl_context.hpp
│   │   │   └── src/
│   │   │       ├── jni_desktop.cpp        # Desktop JNI entry point
│   │   │       └── desktop_gl_context.cpp # GL context creation
│   │   ├── kotlin/app/rive/mp/
│   │   │   ├── RiveFile.desktop.kt        # actual class
│   │   │   ├── Artboard.desktop.kt
│   │   │   ├── RiveCanvas.desktop.kt      # Canvas-based impl
│   │   │   ├── CommandQueue.desktop.kt
│   │   │   └── ...
│   │   └── resources/jnilibs/linux-x86-64/
│   │       └── libmprive-desktop.so       # PRECOMPILED (checked in)
│   │
│   └── nativeInterop/cpp/                  # SHARED C++ CODE
│       ├── include/
│       │   ├── jni_refs.hpp                # JNI reference management
│       │   ├── jni_helpers.hpp             # Type conversions, utilities
│       │   ├── rive_log.hpp                # Logging
│       │   ├── render_buffer.hpp           # Off-screen rendering buffer
│       │   └── rive_bindings.hpp           # Common binding declarations
│       └── src/
│           ├── jni_common/
│           │   ├── jni_refs.cpp
│           │   ├── jni_helpers.cpp
│           │   ├── rive_log.cpp
│           │   └── render_buffer.cpp       # GPU→CPU pixel readback
│           └── bindings/
│               ├── bindings_init.cpp       # JNI_OnLoad
│               ├── bindings_file.cpp       # RiveFile JNI
│               ├── bindings_artboard.cpp   # Artboard JNI
│               ├── bindings_renderer.cpp   # Rendering to buffer
│               ├── bindings_animation.cpp  # LinearAnimation JNI
│               ├── bindings_statemachine.cpp # StateMachine JNI
│               ├── bindings_input.cpp      # Input bindings
│               └── bindings_commandqueue.cpp # CommandQueue JNI
│
└── scripts/
    ├── build_desktop_native.sh
    └── clean_native.sh
```

---

## Implementation Phases

### Phase 1: Foundation - Shared Native Code (Week 1)

**Goal**: Create shared C++ infrastructure for JNI bindings

#### Step 1.1: Create Directory Structure
- [ ] Create `mprive/src/nativeInterop/cpp/include/`
- [ ] Create `mprive/src/nativeInterop/cpp/src/jni_common/`
- [ ] Create `mprive/src/nativeInterop/cpp/src/bindings/`

#### Step 1.2: JNI Common Utilities

**File**: `mprive/src/nativeInterop/cpp/include/jni_refs.hpp`
```cpp
#pragma once
#include <jni.h>

namespace rive_mp {
    // Global JVM pointer
    extern JavaVM* g_JVM;
    
    // JNI class loader
    void InitJNIClassLoader(JNIEnv* env, jobject contextObject);
    
    // Helper to get JNIEnv in any thread
    JNIEnv* GetJNIEnv();
}
```

**File**: `mprive/src/nativeInterop/cpp/src/jni_common/jni_refs.cpp`
- [ ] Implement global JVM storage
- [ ] Implement class loader initialization
- [ ] Implement thread-safe JNIEnv access

**File**: `mprive/src/nativeInterop/cpp/include/jni_helpers.hpp`
```cpp
#pragma once
#include <jni.h>
#include <string>
#include <vector>

namespace rive_mp {
    // Type conversions
    std::string JStringToStdString(JNIEnv* env, jstring jstr);
    jstring StdStringToJString(JNIEnv* env, const std::string& str);
    
    // Array conversions
    std::vector<uint8_t> JByteArrayToVector(JNIEnv* env, jbyteArray arr);
    jbyteArray VectorToJByteArray(JNIEnv* env, const std::vector<uint8_t>& vec);
    
    // Error handling
    void ThrowRiveException(JNIEnv* env, const char* message);
}
```

**File**: `mprive/src/nativeInterop/cpp/src/jni_common/jni_helpers.cpp`
- [ ] Implement all helper functions
- [ ] Add error checking and logging

#### Step 1.3: Logging Infrastructure

**File**: `mprive/src/nativeInterop/cpp/include/rive_log.hpp`
```cpp
#pragma once

namespace rive_mp {
    void RiveLogD(const char* tag, const char* message);
    void RiveLogE(const char* tag, const char* message);
    void InitializeRiveLog();
}
```

**File**: `mprive/src/nativeInterop/cpp/src/jni_common/rive_log.cpp`
- [ ] Implement logging via JNI callback to Kotlin
- [ ] Platform-specific logging (logcat for Android, stdout for Desktop)

#### Step 1.4: Render Buffer (for Canvas/Bitmap approach)

**File**: `mprive/src/nativeInterop/cpp/include/render_buffer.hpp`
```cpp
#pragma once
#include <rive/renderer.hpp>
#include <vector>

namespace rive_mp {
    class RenderBuffer {
    public:
        RenderBuffer(int width, int height);
        ~RenderBuffer();
        
        // Render artboard to buffer
        void render(rive::Artboard* artboard, 
                   rive::Fit fit, 
                   rive::Alignment alignment);
        
        // Get pixel data (RGBA)
        const std::vector<uint8_t>& getPixels() const;
        
    private:
        int m_width;
        int m_height;
        GLuint m_fbo;
        GLuint m_texture;
        std::vector<uint8_t> m_pixels;
    };
}
```

**File**: `mprive/src/nativeInterop/cpp/src/jni_common/render_buffer.cpp`
- [ ] Implement OpenGL FBO creation
- [ ] Implement rendering to FBO
- [ ] Implement pixel readback (glReadPixels)
- [ ] Handle RGBA→BGRA conversion if needed

---

### Phase 2: Minimal JNI Bindings (Week 1-2)

**Goal**: Implement core JNI methods for file loading, rendering, and basic animation

#### Step 2.1: Initialization

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_init.cpp`

```cpp
#include <jni.h>
#include "jni_refs.hpp"
#include "rive_log.hpp"

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    rive_mp::g_JVM = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    rive_mp::g_JVM = nullptr;
}

// Java: app.rive.mp.RiveNative.nativeInit()
JNIEXPORT void JNICALL
Java_app_rive_mp_RiveNative_nativeInit(JNIEnv* env, jobject thiz) {
    rive_mp::InitJNIClassLoader(env, thiz);
    rive_mp::InitializeRiveLog();
    // Initialize rive runtime if needed
}

} // extern "C"
```

- [ ] Implement JNI_OnLoad
- [ ] Implement initialization method
- [ ] Add logging for debugging

#### Step 2.2: RiveFile Bindings

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_file.cpp`

**JNI Methods**:
```cpp
// Load file from bytes
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_nativeLoadFile(JNIEnv* env, jobject, jbyteArray bytes);

// Get artboard count
JNIEXPORT jint JNICALL
Java_app_rive_mp_RiveFile_nativeGetArtboardCount(JNIEnv* env, jobject, jlong filePtr);

// Get artboard by index
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_nativeGetArtboard(JNIEnv* env, jobject, jlong filePtr, jint index);

// Release file
JNIEXPORT void JNICALL
Java_app_rive_mp_RiveFile_nativeRelease(JNIEnv* env, jobject, jlong filePtr);
```

- [ ] Implement file loading from byte array
- [ ] Implement artboard access
- [ ] Add proper error handling
- [ ] Add resource cleanup

#### Step 2.3: Artboard Bindings

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_artboard.cpp`

**JNI Methods**:
```cpp
// Get artboard bounds
JNIEXPORT jobject JNICALL
Java_app_rive_mp_Artboard_nativeGetBounds(JNIEnv* env, jobject, jlong artboardPtr);

// Advance artboard
JNIEXPORT void JNICALL
Java_app_rive_mp_Artboard_nativeAdvance(JNIEnv* env, jobject, jlong artboardPtr, jfloat elapsed);

// Release artboard
JNIEXPORT void JNICALL
Java_app_rive_mp_Artboard_nativeRelease(JNIEnv* env, jobject, jlong artboardPtr);
```

- [ ] Implement artboard methods
- [ ] Handle bounds calculation

#### Step 2.4: Renderer Bindings (Canvas/Bitmap approach)

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_renderer.cpp`

**JNI Methods**:
```cpp
// Render to buffer and return RGBA pixels
JNIEXPORT jbyteArray JNICALL
Java_app_rive_mp_RenderBuffer_nativeRenderToBuffer(
    JNIEnv* env, jobject,
    jlong artboardPtr,
    jint width, jint height,
    jint fit, jint alignment,
    jint clearColor
);
```

- [ ] Implement rendering to off-screen buffer
- [ ] Implement pixel readback
- [ ] Return RGBA byte array

#### Step 2.5: Animation Bindings

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_animation.cpp`

**JNI Methods**:
```cpp
// Get animation count
JNIEXPORT jint JNICALL
Java_app_rive_mp_Artboard_nativeGetAnimationCount(JNIEnv* env, jobject, jlong artboardPtr);

// Create animation instance
JNIEXPORT jlong JNICALL
Java_app_rive_mp_Artboard_nativeCreateAnimation(JNIEnv* env, jobject, jlong artboardPtr, jint index);

// Advance animation
JNIEXPORT jboolean JNICALL
Java_app_rive_mp_Animation_nativeAdvance(JNIEnv* env, jobject, jlong animPtr, jfloat elapsed);

// Apply animation
JNIEXPORT void JNICALL
Java_app_rive_mp_Animation_nativeApply(JNIEnv* env, jobject, jlong animPtr, jlong artboardPtr, jfloat mix);
```

- [ ] Implement animation access
- [ ] Implement animation advancement
- [ ] Implement animation application

#### Step 2.6: StateMachine Bindings

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_statemachine.cpp`

**JNI Methods**:
```cpp
// Get state machine count
JNIEXPORT jint JNICALL
Java_app_rive_mp_Artboard_nativeGetStateMachineCount(JNIEnv* env, jobject, jlong artboardPtr);

// Create state machine
JNIEXPORT jlong JNICALL
Java_app_rive_mp_Artboard_nativeCreateStateMachine(JNIEnv* env, jobject, jlong artboardPtr, jint index);

// Advance state machine
JNIEXPORT jboolean JNICALL
Java_app_rive_mp_StateMachine_nativeAdvance(JNIEnv* env, jobject, jlong smPtr, jfloat elapsed);
```

- [ ] Implement state machine access
- [ ] Implement state machine advancement

#### Step 2.7: Input Bindings

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_input.cpp`

**JNI Methods**:
```cpp
// Get input count
JNIEXPORT jint JNICALL
Java_app_rive_mp_StateMachine_nativeGetInputCount(JNIEnv* env, jobject, jlong smPtr);

// Set boolean input
JNIEXPORT void JNICALL
Java_app_rive_mp_StateMachine_nativeSetBooleanInput(JNIEnv* env, jobject, jlong smPtr, jint index, jboolean value);

// Set number input
JNIEXPORT void JNICALL
Java_app_rive_mp_StateMachine_nativeSetNumberInput(JNIEnv* env, jobject, jlong smPtr, jint index, jfloat value);

// Fire trigger
JNIEXPORT void JNICALL
Java_app_rive_mp_StateMachine_nativeFireTrigger(JNIEnv* env, jobject, jlong smPtr, jint index);
```

- [ ] Implement input access and manipulation

---

### Phase 3: Android Native Build Configuration (Week 2)

**Goal**: Configure CMake build for Android platform

#### Step 3.1: Update Android CMakeLists.txt

**File**: `mprive/src/androidMain/cpp/CMakeLists.txt`

(See full CMakeLists.txt content in appendix - includes Premake invocation, static library linking, etc.)

- [ ] Create and test CMakeLists.txt
- [ ] Verify all paths are correct

#### Step 3.2: Create Android Premake Script

**File**: `mprive/src/androidMain/cpp/premake5_cpp_runtime.lua`

```lua
-- Build Rive C++ Runtime for Android
local path = require('path')
local rive_runtime_dir = os.getenv('RIVE_RUNTIME_DIR')

-- Build the Rive Renderer
dofile(path.join(rive_runtime_dir, 'renderer/premake5_pls_renderer.lua'))
-- Build the Rive C++ Runtime
dofile(path.join(rive_runtime_dir, 'premake5_v2.lua'))

project('rive_cpp_runtime')
do
    kind('StaticLib')
    links({
        'rive',
        'rive_pls_renderer',
        'rive_harfbuzz',
        'rive_sheenbidi',
        'rive_yoga',
        'miniaudio',
    })
end
```

- [ ] Create premake script
- [ ] Test build process

#### Step 3.3: Android-Specific Code

**File**: `mprive/src/androidMain/cpp/src/jni_android.cpp`

```cpp
#include <jni.h>
#include <android/log.h>

// Android-specific initialization or helpers if needed
// Most logic is in shared bindings
```

- [ ] Add any Android-specific helpers (if needed)

#### Step 3.4: Verify Build

- [ ] Build for all ABIs (arm64-v8a, armeabi-v7a, x86_64, x86)
- [ ] Verify .so files are generated
- [ ] Check library dependencies

---

### Phase 4: Desktop Skia Renderer Implementation (Week 2-3) ✨

**Goal**: Implement Skia Renderer for Desktop (GPU-accelerated via Compose Desktop)

#### Step 4.1: Create Desktop CMakeLists.txt

**File**: `mprive/src/desktopMain/cpp/CMakeLists.txt`

**Changes from Android**:
- Link against **Skia Renderer** (`rive_skia_renderer`) instead of PLS
- Link against Skia libraries (from Compose Desktop)
- Use JNI package finding for Desktop JDK

- [ ] Create and test CMakeLists.txt
- [ ] Link against `rive_skia_renderer` library
- [ ] Link against Skia (from Compose Desktop dependencies)

#### Step 4.2: Create Desktop Premake Script

**File**: `mprive/src/desktopMain/cpp/premake5_desktop_runtime.lua`

```lua
-- Build Rive C++ Runtime for Desktop Linux with Skia Renderer
local path = require('path')
local rive_runtime_dir = os.getenv('RIVE_RUNTIME_DIR')

-- Build the Skia Renderer (instead of PLS)
dofile(path.join(rive_runtime_dir, 'skia/renderer/premake5_v2.lua'))
-- Build the Rive C++ Runtime
dofile(path.join(rive_runtime_dir, 'premake5_v2.lua'))

project('rive_cpp_runtime')
do
    kind('StaticLib')
    links({
        'rive',
        'rive_skia_renderer',  -- Use Skia renderer
        'rive_harfbuzz',
        'rive_sheenbidi',
        'rive_yoga',
    })
end
```

- [ ] Create premake script for Linux with Skia renderer
- [ ] Test build process

#### Step 4.3: Desktop Skia Renderer JNI Bindings

**File**: `mprive/src/desktopMain/cpp/include/desktop_skia_renderer.hpp`

```cpp
#pragma once
#include "skia_renderer.hpp"  // From rive-runtime/skia/renderer
#include "SkCanvas.h"

namespace rive_mp {
    // Wrapper for Skia renderer
    class DesktopSkiaRenderer {
    public:
        DesktopSkiaRenderer(SkCanvas* canvas);
        ~DesktopSkiaRenderer();
        
        void render(rive::Artboard* artboard);
        void updateCanvas(SkCanvas* canvas);
        
    private:
        std::unique_ptr<rive::SkiaRenderer> m_renderer;
    };
}
```

**File**: `mprive/src/desktopMain/cpp/src/desktop_skia_renderer.cpp`
- [ ] Implement Skia renderer wrapper
- [ ] Handle canvas updates (canvas can change between frames)
- [ ] Implement artboard rendering

**File**: `mprive/src/desktopMain/cpp/src/jni_desktop.cpp`
```cpp
#include <jni.h>
#include "desktop_skia_renderer.hpp"

extern "C" {
    // Create Skia renderer with Compose Desktop's canvas
    JNIEXPORT jlong JNICALL
    Java_app_rive_mp_NativeRenderSurfaceKt_nativeCreateSkiaSurface(
        JNIEnv* env, jobject, jobject skiaCanvas, jint width, jint height);
    
    // Render to Skia canvas
    JNIEXPORT void JNICALL
    Java_app_rive_mp_NativeRenderSurfaceKt_nativeDrawToSkiaCanvas(
        JNIEnv* env, jobject, jlong handle, jobject skiaCanvas);
}
```

- [ ] Implement JNI bindings for Skia renderer
- [ ] Extract SkCanvas* from Compose Desktop's Skia canvas object
- [ ] Handle thread safety (JNI + Skia)

#### Step 4.4: Build Script

**File**: `mprive/scripts/build_desktop_native.sh`

```bash
#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/src/desktopMain/cpp/build"
OUTPUT_DIR="$PROJECT_ROOT/src/desktopMain/resources/jnilibs/linux-x86-64"

echo "Building mprive desktop native library..."

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DJAVA_HOME=/opt/android-studio/jbr

# Build
cmake --build . --config Release -j$(nproc)

# Copy output
mkdir -p "$OUTPUT_DIR"
cp libmprive-desktop.so "$OUTPUT_DIR/"

echo "✓ Desktop native library built successfully"
echo "  Output: $OUTPUT_DIR/libmprive-desktop.so"
echo ""
echo "⚠️  Please commit this file to source control:"
echo "  git add $OUTPUT_DIR/libmprive-desktop.so"
echo "  git commit -m 'Update desktop native library'"
```

- [ ] Create build script
- [ ] Make executable: `chmod +x build_desktop_native.sh`
- [ ] Test build process
- [ ] Commit libmprive-desktop.so to git

---

### Phase 5: Kotlin Common API (Week 3)

**Goal**: Define platform-agnostic Kotlin API

(Full API definitions in appendix - includes RiveFile, Artboard, Animation, StateMachine, RenderBuffer, etc.)

---

### Phase 6: Android Actual Implementation (Week 3-4)

**Goal**: Implement Android-specific Kotlin code with both rendering approaches

---

### Phase 7: Desktop Actual Implementation (Week 4)

**Goal**: Implement Desktop-specific Kotlin code with Canvas rendering

---

### Phase 8: Performance Profiling & Optimization (Week 5+)

**Goal**: Profile and optimize rendering performance on both platforms

**Note**: With Skia Renderer on Desktop, we already have GPU-accelerated rendering! This phase focuses on profiling and fine-tuning.

#### Profiling & Benchmarking

1. **Performance Metrics**
   - [ ] Measure frame times on Android (target: <2ms)
   - [ ] Measure frame times on Desktop (target: <3ms)
   - [ ] Profile CPU usage during rendering
   - [ ] Profile GPU usage (via platform-specific tools)
   - [ ] Measure memory allocations per frame

2. **Comparative Benchmarks**
   - [ ] Compare Android PLS vs existing kotlin module
   - [ ] Compare Desktop Skia vs hypothetical Canvas/Bitmap approach
   - [ ] Document performance characteristics

#### Optimization Areas

1. **Frame Caching**
   - [ ] Cache rendered frames when content is static
   - [ ] Dirty region tracking
   - [ ] Partial updates

2. **Memory Optimization**
   - [ ] Buffer pooling for reusable resources
   - [ ] Reduce allocations per frame
   - [ ] Native memory management tuning

3. **Rendering Pipeline**
   - [ ] Optimize state changes between frames
   - [ ] Batch rendering operations where possible
   - [ ] Investigate draw call reduction

4. **Platform-Specific Optimizations**
   - **Android**: TextureView configuration, EGL settings
   - **Desktop**: Skia GPU backend configuration, canvas reuse

#### Optional: Advanced Rendering Backends (Future)

**Only if performance targets not met or special requirements arise**:

- [ ] **Desktop PLS + Manual OpenGL**: For maximum performance (if Skia renderer insufficient)
- [ ] **Hardware Texture Sharing**: Investigate zero-copy texture sharing between Rive and Compose
- [ ] **Metal on macOS**: Native Metal renderer for macOS Desktop (future platform)

---

## Testing Strategy

### Unit Tests

**Android**:
- [ ] Test native library loading
- [ ] Test RiveFile loading
- [ ] Test artboard access
- [ ] Test animation advancement
- [ ] Test state machine

**Desktop**:
- [ ] Test native library loading
- [ ] Test RiveFile loading (same as Android)
- [ ] Test GL context creation

### Integration Tests

**Android**:
- [ ] Test RiveUI rendering
- [ ] Test RiveCanvas rendering
- [ ] Test pointer input
- [ ] Test lifecycle handling

**Desktop**:
- [ ] Test RiveCanvas rendering
- [ ] Test frame loop
- [ ] Test resource cleanup

### Visual Tests

- [ ] Create sample app with test animations
- [ ] Verify rendering quality on Android
- [ ] Verify rendering quality on Desktop
- [ ] Compare with kotlin module output

### Performance Tests

- [ ] Measure frame rate (target: 60fps)
- [ ] Measure memory usage
- [ ] Measure CPU usage
- [ ] Benchmark Canvas vs TextureView on Android

---

## Success Criteria

### Phase 1-4 (Native Foundation)
- ✅ Native libraries build successfully for Android (all ABIs)
- ✅ Native library builds successfully for Desktop (Linux x86_64)
- ✅ JNI methods callable from Kotlin
- ✅ Basic file loading and rendering works

### Phase 5-7 (Kotlin Implementation)
- ✅ API is similar to kotlin module
- ✅ PLS rendering works on Android (TextureView)
- ✅ Skia rendering works on Desktop (GPU-accelerated)
- ✅ Frame loop runs smoothly (60fps target)
- ✅ Resources properly cleaned up

### Phase 8 (Optimization)
- ✅ Identified performance bottlenecks
- ✅ Implemented at least 2 optimizations
- ✅ Measured performance improvement
- ✅ Documented findings

---

## Timeline

| Phase | Duration | Key Deliverables |
|-------|----------|------------------|
| 1 | Week 1 | Shared C++ foundation |
| 2 | Week 1-2 | JNI bindings implemented |
| 3 | Week 2 | Android build working |
| 4 | Week 2-3 | Desktop build working |
| 5 | Week 3 | Common Kotlin API defined |
| 6 | Week 3-4 | Android implementation complete |
| 7 | Week 4 | Desktop implementation complete |
| 8 | Week 5+ | Performance optimizations |

**Total Estimated Time**: 4-6 weeks

---

## Risk Mitigation

### Risk: Skia Canvas Access on Desktop
**Description**: Accessing native SkCanvas pointer from Compose Desktop internals  
**Probability**: Medium  
**Impact**: High  
**Mitigation**: 
- Use reflection to access `org.jetbrains.skia.Canvas._ptr` field
- Test across different Compose Desktop versions
- Fallback to Canvas/Bitmap if Skia access fails
- Engage with JetBrains Compose team for official API

### Risk: Skia GPU Backend Not Enabled
**Description**: Skia might use software rendering on some systems  
**Probability**: Low  
**Impact**: Medium  
**Mitigation**:
- Detect Skia backend at runtime (GPU vs Software)
- Warn user if software rendering detected
- Document GPU driver requirements

### Risk: JNI Method Signature Mismatches
**Probability**: Low  
**Mitigation**: Use `javah` or IDE to generate correct signatures

### Risk: Memory Leaks
**Probability**: Low  
**Mitigation**: Implement proper cleanup, use Closeable pattern

### Risk: Performance Not Meeting Targets
**Description**: Skia renderer overhead higher than expected  
**Probability**: Low  
**Impact**: Medium  
**Mitigation**:
- Benchmark early and often
- Profile with real-world animations
- Consider PLS + manual OpenGL as last resort (complex but maximum performance)

---

## Future Enhancements

1. **iOS Support** (after Android + Desktop working)
   - PLS Renderer + MTKView (Metal) for maximum performance
   - Or Skia Renderer via CoreGraphics bridge
2. **WebAssembly Support** 
   - PLS Renderer + WebGL for GPU acceleration
   - Or Skia Renderer via SkiaWasm
3. **macOS Native Desktop** (currently using JVM)
   - Native macOS app with PLS + Metal or Skia + CoreGraphics
4. **Text Run Bindings** (when needed for text manipulation)
5. **Audio Playback** (stub implementation can be completed)
6. **Advanced Rendering** (Custom shaders, effects)
7. **Developer Tools** (Inspector, profiler)

---

## Appendix: Key Files Reference

### C++ Files (Shared)
- `jni_refs.hpp/cpp` - JNI utilities
- `jni_helpers.hpp/cpp` - Type conversions
- `rive_log.hpp/cpp` - Logging
- `render_buffer.hpp/cpp` - Off-screen rendering
- `bindings_*.cpp` - JNI bindings

### Kotlin Files (Common)
- `RiveFile.kt` - File loading API
- `Artboard.kt` - Artboard API
- `Animation.kt` - Animation API
- `StateMachine.kt` - State machine API
- `RiveCanvas.kt` - Canvas rendering composable
- `RiveUI.kt` - UI rendering composable

### Kotlin Files (Android)
- `*.android.kt` - Android actual implementations
- `RiveUI.android.kt` - TextureView-based rendering

### Kotlin Files (Desktop)
- `*.desktop.kt` - Desktop actual implementations
- `NativeLibraryLoader.kt` - Library loading
- `NativeRenderSurface.desktop.kt` - Skia renderer integration
- `RiveUI.desktop.kt` - Skia-based rendering

### Performance Reference

| Platform | Renderer | Frame Time (1080p) | GPU Accelerated | Notes |
|----------|----------|-------------------|-----------------|-------|
| Android | PLS + TextureView | 0.5-2ms | ✅ Yes | Maximum performance |
| Desktop | Skia Renderer | 1-3ms | ✅ Yes (via Skia) | ~30% overhead vs PLS, but excellent |
| Fallback | Canvas/Bitmap | 5-15ms | ❌ No | Avoid - 300-600% slower |

For detailed architecture and implementation of the multiplatform renderer (NativeRenderSurface), see [mprenderer.md](mprenderer.md).

---

**End of Implementation Plan**
