# mprive RiveFile vs kotlin Module RiveFile - Comparison

**Date**: January 1, 2026  
**Status**: Analysis after Step 2.2 completion

---

## Executive Summary

The `mprive` RiveFile implementation is **minimal and functional** but **missing several features** from the original `kotlin` module. This is intentional for Phase 2 (minimal bindings), but important differences should be noted.

**Key Takeaway**: mprive provides core file loading and artboard access, but lacks view models, enums, async loading, and Compose integration present in the kotlin module.

---

## Architecture Comparison

### kotlin Module (Original)
```
RiveFileSource → RiveFile.fromSource() → CommandQueue.loadFile()
                                       → FileHandle wrapper
                                       → Reference counting
```

**Characteristics**:
- **CommandQueue Architecture**: Client/server model with separate render thread
- **FileHandle Wrapper**: Abstract handle, not raw pointer
- **Reference Counting**: `acquire()` / `release()` for lifecycle management
- **AutoCloseable**: Standard Kotlin resource management via `CloseOnce`

### mprive Module (New)
```
ByteArray → RiveFile.load() → nativeLoadFile() JNI → rive::File* (raw pointer)
```

**Characteristics**:
- **Direct JNI**: No intermediary layer, direct native calls
- **Raw Pointer**: Native pointer exposed as `Long`
- **Manual Disposal**: Explicit `dispose()` call required
- **Simple Lifecycle**: No reference counting (yet)

---

## Feature Comparison

| Feature | kotlin Module | mprive Module | Status |
|---------|---------------|---------------|---------|
| **File Loading** | ✅ Async (suspend) | ✅ Blocking | ⚠️ Different |
| **Error Handling** | ✅ Result<T> | ✅ Exceptions | ⚠️ Different |
| **Byte Array Source** | ✅ Yes | ✅ Yes | ✅ Implemented |
| **Raw Resource Source** | ✅ Yes | ❌ No | ❌ Missing |
| **Artboard Access** | ✅ Via Artboard API | ✅ Direct | ✅ Implemented |
| **Artboard Count** | ✅ Via getArtboardNames() | ✅ artboardCount | ✅ Implemented |
| **Get Artboard by Index** | ✅ Yes | ✅ Yes | ✅ Implemented |
| **Get Artboard by Name** | ✅ Yes | ✅ Yes | ✅ Implemented |
| **Get Artboard Names** | ✅ getArtboardNames() | ❌ No | ❌ Missing |
| **View Models** | ✅ Full support | ❌ Not implemented | ❌ Missing |
| **View Model Instances** | ✅ getViewModelInstanceNames() | ❌ Not implemented | ❌ Missing |
| **View Model Properties** | ✅ getViewModelProperties() | ❌ Not implemented | ❌ Missing |
| **Enums** | ✅ getEnums() | ❌ Not implemented | ❌ Missing |
| **Caching** | ✅ SuspendLazy | ❌ None | ❌ Missing |
| **Compose Integration** | ✅ rememberRiveFile() | ❌ None | ❌ Missing |
| **AutoCloseable** | ✅ Yes (CloseOnce) | ❌ No | ❌ Missing |
| **Reference Counting** | ✅ Yes | ❌ Manual only | ❌ Missing |
| **Coroutine Support** | ✅ Suspend functions | ❌ Blocking only | ❌ Missing |

---

## Detailed Feature Analysis

### 1. File Loading

#### kotlin Module
```kotlin
suspend fun fromSource(
    source: RiveFileSource,
    commandQueue: CommandQueue
): Result<RiveFile>

sealed interface RiveFileSource {
    value class Bytes(val data: ByteArray) : RiveFileSource
    data class RawRes(@RawResource val resId: Int, val resources: Resources) : RiveFileSource
}
```

**Features**:
- ✅ Async loading (suspend function)
- ✅ Multiple sources (Bytes, RawRes)
- ✅ Result type (Loading, Success, Error)
- ✅ Cancellation support
- ✅ IO dispatcher for file loading

#### mprive Module
```kotlin
actual class RiveFile private constructor(private var nativePtr: Long) {
    actual companion object {
        actual fun load(bytes: ByteArray): RiveFile {
            val ptr = nativeLoadFile(bytes)
            if (ptr == 0L) throw RiveException("Failed to load file")
            return RiveFile(ptr)
        }
    }
}
```

**Features**:
- ✅ Blocking loading
- ✅ ByteArray source only
- ✅ Exception on error
- ❌ No async support
- ❌ No RawRes support
- ❌ No Result type
- ❌ No cancellation

### 2. Artboard Access

#### kotlin Module
```kotlin
// Indirect via CommandQueue
suspend fun getArtboardNames(): List<String>
// Then use names to create artboards via separate API
```

**Features**:
- ✅ Query artboard names
- ✅ Cached results
- ✅ Async queries

#### mprive Module
```kotlin
actual val artboardCount: Int
actual fun artboard(): Artboard  // default
actual fun artboard(index: Int): Artboard
actual fun artboard(name: String): Artboard
```

**Features**:
- ✅ Direct artboard access
- ✅ Multiple access methods
- ❌ No getArtboardNames() query
- ❌ No caching
- ❌ Blocking only

### 3. View Models

#### kotlin Module
```kotlin
suspend fun getViewModelNames(): List<String>
suspend fun getViewModelInstanceNames(viewModel: String): List<String>
suspend fun getViewModelProperties(viewModel: String): List<Property>

data class Property(
    val name: String,
    val type: PropertyType,
    // ...
)
```

**Features**:
- ✅ Full view model support
- ✅ Instance management
- ✅ Property queries
- ✅ Type information

#### mprive Module
```kotlin
// NOT IMPLEMENTED
```

**Impact**: ❌ **Critical Missing Feature** - View models are essential for advanced Rive animations with data binding.

### 4. Enums

#### kotlin Module
```kotlin
suspend fun getEnums(): List<Enum>

data class Enum(
    val name: String,
    val values: List<String>
)
```

**Features**:
- ✅ Enum queries
- ✅ Enum values

#### mprive Module
```kotlin
// NOT IMPLEMENTED
```

**Impact**: ❌ **Missing Feature** - Required for view model properties that use enums.

### 5. Compose Integration

#### kotlin Module
```kotlin
@Composable
fun rememberRiveFile(
    source: RiveFileSource,
    commandQueue: CommandQueue = rememberCommandQueue(),
): Result<RiveFile>
```

**Features**:
- ✅ Automatic lifecycle management
- ✅ Reactive loading state
- ✅ Auto-cleanup on disposal
- ✅ Idiomatic Compose API

#### mprive Module
```kotlin
// NOT IMPLEMENTED
```

**Impact**: ❌ **Missing Feature** - No Compose-first API. Users must manually manage lifecycle.

### 6. Memory Management

#### kotlin Module
```kotlin
class RiveFile : AutoCloseable by CloseOnce(...) {
    // Automatic cleanup via CloseOnce wrapper
    // Reference counting via commandQueue.acquire()/release()
}
```

**Features**:
- ✅ AutoCloseable interface
- ✅ Safe double-close protection
- ✅ Reference counting
- ✅ Automatic cleanup in Compose

#### mprive Module
```kotlin
actual class RiveFile {
    actual fun dispose() {
        if (nativePtr != 0L) {
            nativeRelease(nativePtr)
            nativePtr = 0L
        }
    }
}
```

**Features**:
- ✅ Manual dispose()
- ✅ Double-dispose safe
- ❌ No AutoCloseable
- ❌ No reference counting
- ❌ No automatic cleanup

---

## What's Missing in mprive?

### ❌ High Priority (Core Features)

1. **View Model Support**
   - **Impact**: Critical - View models are essential for data-driven animations
   - **Required JNI**: `getViewModelNames()`, `getViewModelInstanceNames()`, `getViewModelProperties()`
   - **Effort**: Medium (requires additional JNI bindings)

2. **Enum Support**
   - **Impact**: High - Required for view model properties
   - **Required JNI**: `getEnums()`
   - **Effort**: Low (simple JNI binding)

3. **AutoCloseable Implementation**
   - **Impact**: High - Standard Kotlin resource management
   - **Required Changes**: Implement AutoCloseable, add CloseOnce wrapper
   - **Effort**: Low (Kotlin-only change)

4. **getArtboardNames() Query**
   - **Impact**: Medium - Useful for discovery without creating instances
   - **Required JNI**: `getArtboardNames()`
   - **Effort**: Low (simple JNI binding)

### ⚠️ Medium Priority (Quality of Life)

5. **Async Loading (Suspend Functions)**
   - **Impact**: Medium - Better UX for large files
   - **Required Changes**: Make load() suspend, use coroutines
   - **Effort**: Low (wrapper around existing blocking call)

6. **Result Type for Error Handling**
   - **Impact**: Medium - More idiomatic than exceptions
   - **Required Changes**: Return Result<RiveFile> instead of throwing
   - **Effort**: Low (Kotlin-only change)

7. **RawRes Source Support**
   - **Impact**: Medium - Convenient for Android resources
   - **Required Changes**: Add RiveFileSource sealed interface
   - **Effort**: Low (Kotlin wrapper + resource loading)

8. **Query Result Caching**
   - **Impact**: Medium - Performance optimization
   - **Required Changes**: Add SuspendLazy caching
   - **Effort**: Low (Kotlin-only change)

9. **Compose Integration**
   - **Impact**: Medium - Better Compose DX
   - **Required Changes**: Add rememberRiveFile() composable
   - **Effort**: Low (Kotlin composable wrapper)

### 💡 Low Priority (Advanced)

10. **CommandQueue Architecture**
    - **Impact**: Low - Separate render thread (complex)
    - **Required Changes**: Major refactoring
    - **Effort**: Very High
    - **Note**: May not be needed if direct JNI is performant enough

---

## API Comparison Examples

### Loading a File

#### kotlin Module
```kotlin
@Composable
fun MyScreen() {
    val fileResult = rememberRiveFile(
        source = RiveFileSource.RawRes.from(R.raw.my_animation)
    )
    
    when (fileResult) {
        is Result.Loading -> CircularProgressIndicator()
        is Result.Error -> Text("Error: ${fileResult.error}")
        is Result.Success -> {
            val file = fileResult.value
            // Use file...
            // Auto-disposed when MyScreen leaves composition
        }
    }
}
```

#### mprive Module
```kotlin
@Composable
fun MyScreen() {
    val fileResult = remember {
        try {
            val bytes = context.resources.openRawResource(R.raw.my_animation)
                .use { it.readBytes() }
            Result.success(RiveFile.load(bytes))
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    DisposableEffect(fileResult) {
        onDispose {
            fileResult.getOrNull()?.dispose() // Manual disposal
        }
    }
    
    fileResult.fold(
        onSuccess = { file -> /* Use file */ },
        onFailure = { error -> Text("Error: $error") }
    )
}
```

### Querying Artboards

#### kotlin Module
```kotlin
val artboardNames = file.getArtboardNames() // ["Artboard1", "Artboard2", ...]
```

#### mprive Module
```kotlin
// NOT AVAILABLE - must iterate by index:
val artboardNames = (0 until file.artboardCount).map { index ->
    file.artboard(index).use { artboard ->
        // No artboard.name property yet (Step 2.3)
        "Artboard $index"
    }
}
```

### View Models

#### kotlin Module
```kotlin
val viewModels = file.getViewModelNames()
val instances = file.getViewModelInstanceNames("MyViewModel")
val properties = file.getViewModelProperties("MyViewModel")
```

#### mprive Module
```kotlin
// NOT AVAILABLE
```

---

## Recommendations

### For mprive Development

1. **Immediate** (Before releasing):
   - ✅ Implement `AutoCloseable` interface
   - ✅ Add `getArtboardNames()` JNI binding
   - ✅ Add suspend wrapper for `load()`

2. **Phase 2+** (After basic rendering works):
   - Add view model support (critical for advanced features)
   - Add enum support
   - Add Result type error handling
   - Add RawRes source support

3. **Phase 3+** (Polish):
   - Add Compose integration (`rememberRiveFile()`)
   - Add query caching
   - Consider reference counting

4. **Future** (Optional):
   - Evaluate CommandQueue architecture (only if needed for performance)

### For Users

**Use kotlin module if**:
- ✅ Need view models
- ✅ Need Compose integration
- ✅ Want automatic lifecycle management
- ✅ Building Android-only app

**Use mprive module if**:
- ✅ Need multiplatform support (Desktop, iOS, wasmJS)
- ✅ Basic animations (no view models)
- ✅ Willing to manage lifecycle manually
- ✅ Want simpler, more direct API

---

## Conclusion

The mprive RiveFile implementation successfully provides **core file loading and artboard access** in a **multiplatform-friendly** way. However, it's currently **missing several features** from the kotlin module:

**Implemented** ✅:
- File loading from byte array
- Artboard access (by index, by name, default)
- Artboard count
- Basic disposal

**Missing** ❌:
- View models (critical)
- Enums
- getArtboardNames() query
- AutoCloseable
- Async loading
- Result type
- RawRes source
- Caching
- Compose integration

This is **acceptable for Phase 2** (minimal bindings), but **additional features should be prioritized** before considering mprive production-ready. The missing features align with the implementation plan's phased approach.

---

**End of Comparison Document**
