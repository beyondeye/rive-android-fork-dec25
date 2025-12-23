# How to Compile and Run Local Changes

This guide explains how to set up your development environment to compile and run the rive-android project with local modifications to the `:kotlin` module.

## Prerequisites

### Required Build Tools

The project uses native C++ code that requires these build tools:

1. **Ninja** - Fast build system
   ```bash
   # Ubuntu/Debian
   sudo apt install ninja-build
   
   # macOS
   brew install ninja

   # Or it is also found in omarchy official packages
     
   # Verify installation
   which ninja
   ```

2. **Premake5** - Project file generator
   ```bash
   # Download from https://premake.github.io/download
   # Or it is also found in omarchy official packages
   # Extract and add to PATH
   
   # Verify installation (actually premake not premake5 on omarchy)
   which premake5
   ```

3. **Android NDK** - Should be installed via Android Studio
   - Required version: `27.2.12479018` (as specified in `kotlin/build.gradle`)
   - Install via: Android Studio → Settings → Languages & Frameworks → Android SDK → SDK Tools → NDK

### Git Submodules

The project depends on the `rive-runtime` submodule which contains the C++ renderer code:

```bash
# Initialize and update all submodules
git submodule update --init --recursive
```

**Important:** If you see CMake errors like `Script returned with error: '' - '127'`, it usually means:
- The submodule is not initialized (empty `submodules/rive-runtime` directory)
- Or required tools (ninja, premake5) are not installed

## Build Variants Configuration

### Understanding Build Variants

The `app` module has three build variants with different dependencies:

| Build Variant | Kotlin Module Dependency | Use Case |
|---------------|-------------------------|----------|
| `debug` | Local `:kotlin` project | **Local development** |
| `release` | Local `:kotlin` project | Release builds |
| `preview` | Maven `libs.rive.android.preview` | Testing published library |

### Important: Use `debug` for Local Development

If you're making changes to the `:kotlin` module (e.g., adding new classes), you **must** use the `debug` or `release` build variant. The `preview` variant uses a pre-built library from Maven that won't include your local changes.

From `app/build.gradle`:
```groovy
debugImplementation project(path: ":kotlin")      // Uses local project
releaseImplementation project(path: ":kotlin")    // Uses local project
previewImplementation(libs.rive.android.preview)  // Uses Maven library ❌
```

## Android Studio Setup

### 1. Configure Build Variants

1. Open **Build Variants** panel: View → Tool Windows → Build Variants
2. Set the following:

| Module | Build Variant | Active ABI |
|--------|---------------|------------|
| `:app` | `debug` | (inherited from kotlin) |
| `:kotlin` | `debug` | See below |

### 2. Select Active ABI

For the `:kotlin` module, select an ABI that matches your target device:

| Target Device | Active ABI |
|---------------|------------|
| Most modern phones (2016+) | `arm64-v8a` |
| Android Emulator (default) | `x86_64` |
| Older 32-bit phones | `armeabi-v7a` |
| Older 32-bit emulators | `x86` |

**Tip:** Selecting a single ABI significantly speeds up native build times.

### 3. Sync Gradle

After changing build variants: File → Sync Project with Gradle Files

## Common Issues & Solutions

### Issue 1: Unresolved Reference Errors

**Symptom:**
```
e: Unresolved reference: sprites
e: Unresolved reference: RiveSprite
e: Unresolved reference: rememberRiveSpriteScene
```

**Cause:** You're using the `preview` build variant which uses the Maven library instead of your local `:kotlin` module.

**Solution:** Switch to `debug` build variant.

### Issue 2: CMake Error 127 (Command Not Found)

**Symptom:**
```
CMake Error at CMakeLists.txt:128 (message):
Script returned with error: '' - '127'
```

**Cause:** Either:
- Git submodule `rive-runtime` is not initialized
- Required build tools (ninja, premake5) are not installed

**Solution:**
```bash
# Initialize submodules
git submodule update --init --recursive

# Install ninja
sudo apt install ninja-build  # or brew install ninja

# Install premake5
# Download from https://premake.github.io/download
```

### Issue 3: NDK Not Found

**Symptom:** Gradle sync fails with NDK-related errors

**Solution:** Install the correct NDK version via Android Studio:
1. File → Settings → Languages & Frameworks → Android SDK
2. SDK Tools tab
3. Check "Show Package Details"
4. Install NDK version `27.2.12479018`

## Quick Setup Checklist

- [ ] Clone repository
- [ ] Initialize submodules: `git submodule update --init --recursive`
- [ ] Install ninja: `sudo apt install ninja-build`
- [ ] Install premake5 (download from https://premake.github.io/download)
- [ ] Install NDK 27.2.12479018 via Android Studio
- [ ] Set build variant to `debug` for both `:app` and `:kotlin` modules
- [ ] Select appropriate Active ABI for `:kotlin` module
- [ ] Sync Gradle project

## Building from Command Line

```bash
# Use the Android Studio JDK
export JAVA_HOME=/opt/android-studio/jbr

# Build debug variant
./gradlew app:assembleDebug

# Run on connected device
./gradlew app:installDebug
```

## Additional Resources

- [Rive Android Documentation](https://rive.app/docs)
- [Android NDK Documentation](https://developer.android.com/ndk)
- [Premake Documentation](https://premake.github.io/docs/)
