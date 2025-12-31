# Implementation Plan: Compose Multiplatform Rendering (Android + Desktop)

**Project**: rive-android-fork-dec25  
**Module**: mprive (Kotlin Multiplatform)  
**Target Platforms**: Android, Desktop/Linux JVM  
**Date**: December 31, 2025  
**Status**: Planning Phase

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
- **Android**: Two rendering approaches (TextureView + Canvas/Bitmap)
- **Desktop**: Canvas/Bitmap rendering
- **API**: Similar to existing kotlin module (package: `app.rive.mp`)
- **Focus**: Compose Multiplatform only (no View-based components)
- **Audio**: Included for both platforms
- **Renderer**: rive_pls_renderer with OpenGL/GLES

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

#### Android: Dual-Mode Rendering

**Mode 1: TextureView (RiveUI Component)**
- **Use Case**: High-performance, native Android rendering
- **How**: TextureView → SurfaceTexture → GPU Surface → Direct rendering
- **Pros**: Zero-copy, maximum performance
- **Cons**: Android-specific, requires more complex setup
- **API**: `RiveUI()` composable (similar to kotlin module)

**Mode 2: Canvas/Bitmap (RiveCanvas Component)**
- **Use Case**: Cross-platform compatible, simpler
- **How**: Off-screen GPU → Pixel readback → Bitmap → Compose Canvas
- **Pros**: Same code as Desktop, simpler debugging
- **Cons**: GPU→CPU copy overhead (~1-5ms per frame)
- **API**: `RiveCanvas()` composable or `drawRive()` in Canvas scope

#### Desktop: Canvas/Bitmap Only

**Approach**: Off-screen OpenGL rendering
- Create off-screen GL context (EGL/GLX)
- Render to FBO (Framebuffer Object)
- Read pixels back to ByteArray
- Convert to ImageBitmap
- Draw to Compose Desktop Canvas

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

### Phase 4: Desktop Native Build Configuration (Week 2-3)

**Goal**: Configure CMake build for Desktop/Linux platform

#### Step 4.1: Create Desktop CMakeLists.txt

**File**: `mprive/src/desktopMain/cpp/CMakeLists.txt`

(See full CMakeLists.txt content in appendix - uses OpenGL instead of GLES, JNI package finding, etc.)

- [ ] Create and test CMakeLists.txt
- [ ] Handle OpenGL vs GLES differences

#### Step 4.2: Create Desktop Premake Script

**File**: `mprive/src/desktopMain/cpp/premake5_desktop_runtime.lua`

```lua
-- Build Rive C++ Runtime for Desktop Linux
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
    })
end
```

- [ ] Create premake script for Linux
- [ ] Test build process

#### Step 4.3: Desktop-Specific Code

**File**: `mprive/src/desktopMain/cpp/include/desktop_gl_context.hpp`

```cpp
#pragma once
#include <GL/gl.h>

namespace rive_mp {
    class DesktopGLContext {
    public:
        DesktopGLContext();
        ~DesktopGLContext();
        
        void makeCurrent();
        void release();
        
    private:
        // EGL or GLX context handles
        void* m_display;
        void* m_context;
        void* m_surface;
    };
}
```

**File**: `mprive/src/desktopMain/cpp/src/desktop_gl_context.cpp`
- [ ] Implement off-screen GL context creation
- [ ] Use EGL or GLX (prefer EGL for headless)
- [ ] Handle context activation/deactivation

**File**: `mprive/src/desktopMain/cpp/src/jni_desktop.cpp`
```cpp
#include <jni.h>
// Desktop-specific initialization
```

- [ ] Add desktop-specific helpers

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

### Phase 8: Performance Optimization (Week 5+)

**Goal**: Investigate and implement optimizations for Canvas/Bitmap rendering

#### Investigation Areas

1. **Reduce Pixel Copying**
   - [ ] Direct GPU texture access from Compose?
   - [ ] Reuse bitmap instances across frames
   - [ ] Optimize RGBA ↔ BGRA conversion
   - [ ] Use native buffer sharing if possible

2. **GPU-Accelerated Blitting**
   - [ ] Use Skia GPU backend on Desktop
   - [ ] Use hardware bitmaps on Android
   - [ ] Investigate Vulkan/Metal support

3. **Frame Caching**
   - [ ] Cache rendered frames when content is static
   - [ ] Dirty region tracking
   - [ ] Partial updates

4. **Memory Optimization**
   - [ ] Buffer pooling
   - [ ] Reduce allocations per frame
   - [ ] Native memory management

#### Optimization Experiments

**Experiment 1: Direct Texture Access**
- [ ] Research Compose internals for direct GPU texture
- [ ] Prototype direct rendering to Compose surface
- [ ] Benchmark performance gains

**Experiment 2: Native ImageBitmap**
- [ ] Create ImageBitmap directly from native code
- [ ] Avoid CPU→GPU copy
- [ ] Test on Android and Desktop

**Experiment 3: Skia Integration**
- [ ] Investigate Skia Picture recording
- [ ] Direct Rive → Skia rendering
- [ ] Benchmark vs. current approach

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
- ✅ Both rendering approaches work on Android
- ✅ Canvas rendering works on Desktop
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

### Risk: OpenGL Context Creation on Desktop
**Mitigation**: Use EGL for headless rendering (works without X11)

### Risk: JNI Method Signature Mismatches
**Mitigation**: Use `javah` or IDE to generate correct signatures

### Risk: Memory Leaks
**Mitigation**: Implement proper cleanup, use Closeable pattern

### Risk: Performance Issues with Canvas Approach
**Mitigation**: Phase 8 optimizations, consider hybrid approach

---

## Future Enhancements

1. **iOS Support** (after Android + Desktop working)
2. **WebAssembly Support** (Canvas rendering already compatible)
3. **Text Run Bindings** (when needed for text manipulation)
4. **Audio Playback** (stub implementation can be completed)
5. **Advanced Rendering** (Custom shaders, effects)
6. **Developer Tools** (Inspector, profiler)

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
- `RiveCanvas.desktop.kt` - Skia-based rendering

---

**End of Implementation Plan**
