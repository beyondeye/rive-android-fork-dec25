# mprive Image Decoders Plan

**Date**: January 19, 2026
**Status**: Planning
**Related**: t4_solve_greybox_rendering.md, mprive_platform_support.md

---

## Motivation

### The Problem

When loading Rive files, embedded images need to be decoded into GPU textures. The `rive::Factory::decodeImage()` method is called for each embedded image during file import. If image decoding fails or returns null, the images become invisible even though the rest of the animation renders correctly.

In mprive, we currently use `riveContext.get()` directly as the factory, which relies on rive-runtime's built-in decoders. However:

1. **Android**: The built-in decoders (libpng, libjpeg, libwebp) require these libraries to be properly built and linked. If they're not available, images fail silently.
2. **Original rive-android**: Uses Android's `BitmapFactory` through JNI, which is more reliable and handles all image formats natively.

### Why Platform-Native Decoders Are Better

| Aspect | Platform-Native | Rive-Runtime Decoders |
|--------|-----------------|----------------------|
| **Binary Size** | No additional dependencies | Adds ~500KB+ (libpng, libjpeg, libwebp) |
| **Format Support** | All platform-supported formats | Only PNG, JPEG, WebP |
| **Reliability** | Battle-tested platform code | May have edge cases |
| **Performance** | Hardware-accelerated on some platforms | Pure software decode |
| **Maintenance** | Automatically updated by OS | Needs manual updates |

---

## Platform Architecture Overview

### Platform-Specific Approaches

| Platform | Recommended Approach | Native API | Fallback |
|----------|---------------------|------------|----------|
| **Android** | Platform-native | BitmapFactory (JNI) | rive-runtime decoders |
| **iOS** | Platform-native (built-in) | CoreGraphics | rive-runtime decoders |
| **macOS** | Platform-native (built-in) | CoreGraphics | rive-runtime decoders |
| **Desktop (Linux)** | rive-runtime decoders | N/A | - |
| **Desktop (Windows)** | rive-runtime decoders | (Could use WIC) | - |
| **WASM/Web** | Browser-native (built-in) | Browser Image API | - |

### Rive-Runtime Built-in Support

The rive-runtime already has platform-native decoders for:

1. **Apple Platforms (iOS, macOS, tvOS)**: Uses `CoreGraphics` via `cg_image_decode()` in `bitmap_decoder_thirdparty.cpp`
2. **WASM/Web**: Uses browser's built-in image decoders with async support

For these platforms, the rive-runtime decoders work correctly out of the box. Only **Android** and **Desktop Linux** need additional work.

---

## Architecture Design

### Factory Wrapper Pattern

Create a platform-specific `FactoryWrapper` that wraps the GPU render context and overrides `decodeImage()` for platform-native decoding:

```
┌─────────────────────────────────────────────────────────────────┐
│                     RenderContext                                │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              rive::gpu::RenderContext                    │   │
│  │  (inherits from rive::Factory)                          │   │
│  │                                                          │   │
│  │  - decodeImage()  ← Uses built-in decoders              │   │
│  │  - makeRenderBuffer()                                    │   │
│  │  - makePath(), makePaint(), etc.                        │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  FactoryWrapper (Platform-Specific)              │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Android: AndroidFactory                     │   │
│  │                                                          │   │
│  │  - decodeImage() → BitmapFactory (JNI)                  │   │
│  │  - Other methods → delegate to riveContext              │   │
│  └─────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              iOS/macOS: (Not needed)                     │   │
│  │              Uses rive-runtime's CoreGraphics           │   │
│  └─────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Desktop: (Uses rive-runtime decoders)       │   │
│  │              Or wraps platform-native (optional)         │   │
│  └─────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              WASM: (Not needed)                          │   │
│  │              Uses browser's built-in decoders           │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### Class Hierarchy

```cpp
// Base class in render_context.hpp
class RenderContext {
public:
    virtual rive::Factory* getFactory() const {
        return riveContext ? riveContext.get() : nullptr;
    }
    // ...
};

// Android-specific in render_context_android.hpp
class RenderContextGL : RenderContext {
public:
    rive::Factory* getFactory() const override {
        return m_androidFactory.get();  // Returns AndroidFactory wrapper
    }
private:
    std::unique_ptr<AndroidFactory> m_androidFactory;
};

// iOS/macOS - uses base class, no override needed (CoreGraphics built-in)
// Desktop - uses base class, with rive-runtime decoders linked
// WASM - uses base class (browser decoders built-in)
```

---

## Platform-Specific Implementation Details

### 1. Android Implementation

**Approach**: Platform-native using `BitmapFactory` through JNI

**Why BitmapFactory**:
- Handles all Android-supported formats (PNG, JPEG, WebP, GIF, HEIF, AVIF)
- Hardware-accelerated on modern devices
- Zero additional binary size
- Already proven in original rive-android

**Files to Create/Modify**:

| File | Action | Description |
|------|--------|-------------|
| `mprive/src/androidMain/cpp/helpers/image_decode.hpp` | Create | Header for Android image decoding |
| `mprive/src/androidMain/cpp/helpers/image_decode.cpp` | Create | Implementation using BitmapFactory JNI |
| `mprive/src/androidMain/cpp/helpers/android_factory.hpp` | Create | Factory wrapper class |
| `mprive/src/androidMain/cpp/helpers/android_factory.cpp` | Create | Factory wrapper implementation |
| `mprive/src/nativeInterop/cpp/include/render_context.hpp` | Modify | Add `createFactory()` method |

**Implementation Steps**:

1. Copy/adapt `kotlin/src/main/cpp/include/helpers/image_decode.hpp` to mprive
2. Copy/adapt `kotlin/src/main/cpp/src/helpers/image_decode.cpp` to mprive
3. Create `AndroidFactory` wrapper class that:
   - Overrides `decodeImage()` to use `renderImageFromAndroidDecode()`
   - Delegates other methods to `riveContext`
4. Modify `RenderContextGL::getFactory()` to return the `AndroidFactory`

### 2. iOS Implementation

**Approach**: Use rive-runtime built-in CoreGraphics decoder (no changes needed)

**Why No Changes Needed**:
- `bitmap_decoder_thirdparty.cpp` already implements `cg_image_decode()` using CoreGraphics
- This is automatically used on Apple platforms via `#ifdef __APPLE__`
- Supports PNG, JPEG, WebP, HEIF, and more

**Recommendation**: ✅ **Use rive-runtime decoders (built-in CoreGraphics)**

The rive-runtime's Apple implementation is production-ready and used by rive-ios.

### 3. macOS/Desktop Apple Implementation

**Approach**: Same as iOS - rive-runtime built-in CoreGraphics

**Recommendation**: ✅ **Use rive-runtime decoders (built-in CoreGraphics)**

### 4. Desktop Linux Implementation

**Approach**: Use rive-runtime decoders (libpng, libjpeg, libwebp)

**Why rive-runtime Decoders**:
- Linux doesn't have a universal native image decoder API
- libpng, libjpeg, libwebp are standard on Linux
- Can link statically to avoid runtime dependencies

**Build Configuration**:
```cmake
# Enable decoders in CMakeLists.txt
add_definitions(-DRIVE_PNG -DRIVE_JPEG -DRIVE_WEBP)

# Link decoder libraries
target_link_libraries(mprive_native
    png
    jpeg
    webp
)
```

**Recommendation**: ✅ **Use rive-runtime decoders with linked libraries**

### 5. Desktop Windows Implementation

**Approach**: Two options

**Option A**: Use rive-runtime decoders (Recommended for simplicity)
- Same as Linux, link libpng, libjpeg, libwebp

**Option B**: Use Windows Imaging Component (WIC)
- Native Windows API for image decoding
- Supports all Windows-supported formats
- Would require custom implementation similar to Android's

**Recommendation**: ✅ **Use rive-runtime decoders** (simpler, consistent with Linux)

### 6. WASM/Web Implementation

**Approach**: Use rive-runtime built-in browser decoder (no changes needed)

**Why No Changes Needed**:
- Emscripten/WASM uses browser's `Image` API for decoding
- The rive-runtime has special `__EMSCRIPTEN__` handling in `image_asset.cpp`
- Supports async decoding via `decodedAsync()` callback

**Key Features**:
- Async image decoding (non-blocking)
- Supports all browser-supported formats
- Zero additional binary size

**Recommendation**: ✅ **Use rive-runtime's built-in browser decoders**

---

## Implementation Plan

### Phase 1: Android Fix (Priority - Unblocks Development)

**Goal**: Fix the grey box rendering issue on Android

**Status**: ✅ Code Complete (as of January 20, 2026)

**Steps**:

1. [x] Create `mprive/src/androidMain/cpp/helpers/` directory
2. [x] Copy and adapt `image_decode.hpp` from kotlin module
3. [x] Copy and adapt `image_decode.cpp` from kotlin module
4. [x] Create `AndroidFactory` wrapper class (`android_factory.hpp`, `android_factory.cpp`)
5. [x] Modify `RenderContextGL::getFactory()` to return `AndroidFactory`
6. [x] Update `CMakeLists.txt` to include new files
7. [x] Create `ImageDecoder.kt` Kotlin class for JNI (in `app/rive/mp/core/`)
8. [x] Create `render_context_android.cpp` with `createAndroidFactory()` implementation
9. [ ] Test with `off_road_car_blog.riv` (contains images)

**Implementation Files Created**:
- `mprive/src/androidMain/cpp/helpers/image_decode.hpp` - Header for Android image decoding
- `mprive/src/androidMain/cpp/helpers/image_decode.cpp` - BitmapFactory JNI implementation
- `mprive/src/androidMain/cpp/helpers/android_factory.hpp` - Factory wrapper header
- `mprive/src/androidMain/cpp/helpers/android_factory.cpp` - Factory wrapper implementation
- `mprive/src/androidMain/cpp/helpers/render_context_android.cpp` - Android-specific RenderContext methods
- `mprive/src/androidMain/kotlin/app/rive/mp/core/ImageDecoder.kt` - Kotlin JNI bridge

**Estimated Effort**: 4-6 hours (Actual: Complete)

### Phase 2: Desktop/Linux Support

**Goal**: Enable image rendering on Linux desktop

**Steps**:

1. [ ] Update CMakeLists.txt to define `RIVE_PNG`, `RIVE_JPEG`, `RIVE_WEBP`
2. [ ] Link against decoder libraries (libpng, libjpeg, libwebp)
3. [ ] Test with sample Rive files

**Estimated Effort**: 2-3 hours

### Phase 3: Verify iOS/WASM (No Code Changes Expected)

**Goal**: Confirm rive-runtime decoders work correctly

**Steps**:

1. [ ] Build and test on iOS simulator
2. [ ] Build and test WASM in browser
3. [ ] Document any issues found

**Estimated Effort**: 2-4 hours (testing only)

---

## File Structure After Implementation

```
mprive/
├── src/
│   ├── androidMain/
│   │   └── cpp/
│   │       └── helpers/
│   │           ├── image_decode.hpp       # Android image decoding header
│   │           ├── image_decode.cpp       # BitmapFactory JNI implementation
│   │           ├── android_factory.hpp    # Factory wrapper header
│   │           └── android_factory.cpp    # Factory wrapper implementation
│   └── nativeInterop/
│       └── cpp/
│           ├── include/
│           │   └── render_context.hpp     # Modified: createFactory() method
│           └── src/
│               └── command_server/
│                   └── command_server_file.cpp  # Modified: use getFactory()
```

---

## Testing Strategy

### Test Files

| File | Description | Has Images |
|------|-------------|------------|
| `off_road_car_blog.riv` | Complex animation | Yes |
| `rating.riv` | Interactive animation | Likely |
| `marty.riv` | Character animation | Likely |

### Test Scenarios

1. **Basic Image Rendering**: Load a .riv file with embedded PNG images
2. **Multiple Image Formats**: Test PNG, JPEG, WebP images
3. **Animated Images**: Test files with image sequences
4. **Large Images**: Test memory handling with high-resolution images
5. **Missing Images**: Verify graceful fallback when decoding fails

---

## Summary of Recommendations

| Platform | Approach | Implementation Needed |
|----------|----------|----------------------|
| **Android** | BitmapFactory (JNI) | Yes - create wrapper |
| **iOS** | CoreGraphics (built-in) | No - works out of box |
| **macOS** | CoreGraphics (built-in) | No - works out of box |
| **Linux** | rive-runtime decoders | Yes - link libraries |
| **Windows** | rive-runtime decoders | Yes - link libraries |
| **WASM** | Browser decoders (built-in) | No - works out of box |

---

## References

- `kotlin/src/main/cpp/include/helpers/image_decode.hpp` - Original Android implementation
- `kotlin/src/main/cpp/src/helpers/image_decode.cpp` - BitmapFactory JNI code
- `submodules/rive-runtime/decoders/src/bitmap_decoder_thirdparty.cpp` - Platform decoders
- `submodules/rive-runtime/src/assets/image_asset.cpp` - WASM async handling

---

**Last Updated**: January 19, 2026