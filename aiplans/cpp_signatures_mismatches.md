# JNI Signature Mismatches: mprive vs kotlin/src/main Reference

## Background

The `mprive` module is intended to be a Kotlin Multiplatform port of the original `rive-android` implementation found in `kotlin/src/main`. The API, architecture, and code in mprive should match the reference implementation as closely as possible, only deviating where necessary for KMP compatibility.

**Critical Issue:** The RiveDemo in mpapp module shows a gray background with nothing drawn. Investigation revealed JNI signature mismatches between the mprive Kotlin bridge and its C++ bindings, causing parameters to be misinterpreted at runtime.

## Root Cause

The mprive module's `CommandQueueBridge` interface was designed with different method signatures than the C++ JNI implementations expect. More critically, **both the mprive Kotlin bridge AND the mprive C++ bindings differ from the kotlin/src/main reference implementation**.

### Example: cppAdvanceStateMachine

**kotlin/src/main Reference (CORRECT):**
```kotlin
// CommandQueueBridge.kt
fun cppAdvanceStateMachine(
    pointer: Long,
    stateMachineHandle: Long,
    deltaTimeNs: Long  // nanoseconds as Long
)
```

**mprive Kotlin Bridge:**
```kotlin
fun cppAdvanceStateMachine(pointer: Long, stateMachineHandle: Long, deltaTimeNs: Long)
```

**mprive C++ JNI:**
```cpp
// Expected by C++: (ptr, requestID, smHandle, deltaTimeSeconds: Float)
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAdvanceStateMachine(
    jlong ptr,
    jlong requestID,      // <-- C++ expects requestID here!
    jlong smHandle,       // <-- C++ expects smHandle here!
    jfloat deltaTimeSeconds  // <-- Float seconds, not Long nanoseconds!
)
```

**Runtime Effect:**
When Kotlin calls `cppAdvanceStateMachine(cppPointer, smHandle=8508657, deltaTimeNs=549326796480)`:
- C++ receives: `requestID=8508657` (was supposed to be smHandle)
- C++ receives: `smHandle=549326796480` (was supposed to be deltaTimeNs)
- State machine lookup fails with invalid handle!

---

## Comprehensive Method Comparison

### Legend
- ✅ = Matches reference
- ⚠️ = Minor difference (may work)
- ❌ = Critical mismatch (causes failures)

---

### Core Lifecycle Methods

| Method | Reference (kotlin/src/main) | mprive Kotlin | mprive C++ | Status |
|--------|---------------------------|---------------|------------|--------|
| `cppConstructor` | `(renderContextPointer: Long): Long` | Same | Same | ✅ |
| `cppDelete` | `(pointer: Long)` | Same | Same | ✅ |
| `cppCreateListeners` | `(pointer: Long, receiver: CommandQueue): Listeners` | Same | Same | ✅ |
| `cppPollMessages` | `(pointer: Long)` | `(pointer: Long, receiver: CommandQueue)` | Needs receiver | ⚠️ |

---

### State Machine Operations

| Method | Reference (kotlin/src/main) | mprive Kotlin | mprive C++ | Status |
|--------|---------------------------|---------------|------------|--------|
| `cppCreateDefaultStateMachine` | `(ptr, requestID, artboardHandle): Long` | Same | Same | ✅ |
| `cppCreateStateMachineByName` | `(ptr, requestID, artboardHandle, name): Long` | Same | Same | ✅ |
| `cppDeleteStateMachine` | `(ptr, requestID, smHandle)` | Same | Same | ✅ |
| `cppAdvanceStateMachine` | `(ptr, smHandle, deltaTimeNs: Long)` | `(ptr, smHandle, deltaTimeNs: Long)` | `(ptr, requestID, smHandle, deltaTime: Float)` | ❌ **CRITICAL** |

**Fix for cppAdvanceStateMachine:**
- mprive C++ should match reference: Remove requestID, use Long nanoseconds

---

### Drawing Operations

| Method | Reference (kotlin/src/main) | mprive Kotlin | mprive C++ | Status |
|--------|---------------------------|---------------|------------|--------|
| `cppDraw` | `(ptr, renderCtxPtr, surfacePtr, drawKey, artboardH, smH, renderTargetPtr, w, h, fit: Byte, align: Byte, scale, clearColor)` | `(ptr, requestID, artboardH, smH, surfacePtr, renderTargetPtr, drawKey, w, h, fit: Int, align: Int, clearColor, scale)` | Matches mprive Kotlin | ❌ **CRITICAL** |

**Issues with cppDraw:**
1. Reference uses `renderContextPointer`, mprive uses `requestID`
2. Parameter ORDER is completely different
3. `fit` and `alignment` are `Byte` in reference, `Int` in mprive
4. `clearColor` and `scaleFactor` are swapped

---

### Property Operations (ViewModel)

| Method | Reference (kotlin/src/main) | mprive Kotlin | mprive C++ | Status |
|--------|---------------------------|---------------|------------|--------|
| `cppSetNumberProperty` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, requestID, vmiHandle, propertyPath, value)` | ❌ |
| `cppSetStringProperty` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, requestID, vmiHandle, propertyPath, value)` | ❌ |
| `cppSetBooleanProperty` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, requestID, vmiHandle, propertyPath, value)` | ❌ |
| `cppSetEnumProperty` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, requestID, vmiHandle, propertyPath, value)` | ❌ |
| `cppSetColorProperty` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, vmiHandle, propertyPath, value)` | `(ptr, requestID, vmiHandle, propertyPath, value)` | ❌ |
| `cppFireTriggerProperty` | `(ptr, vmiHandle, propertyPath)` | `(ptr, vmiHandle, propertyPath)` | `(ptr, requestID, vmiHandle, propertyPath)` | ❌ |

**Pattern:** mprive C++ added `requestID` parameter that the reference doesn't have. Fix by removing requestID from C++.

---

### Asset Operations

| Method | Reference (kotlin/src/main) | mprive Kotlin | mprive C++ | Status |
|--------|---------------------------|---------------|------------|--------|
| `cppDeleteImage` | `(ptr, imageHandle)` | `(ptr, imageHandle)` | `(ptr, requestID, imageHandle)` | ❌ |
| `cppRegisterImage` | `(ptr, name, imageHandle)` | `(ptr, name, imageHandle)` | `(ptr, requestID, fileHandle, assetName, imageHandle)` | ❌ |
| `cppUnregisterImage` | `(ptr, name)` | `(ptr, name)` | `(ptr, requestID, fileHandle, assetName)` | ❌ |

Same pattern for Audio and Font operations.

---

## Priority Fix List

### P0 - Critical (Causing Demo Failure)

1. **`cppAdvanceStateMachine`** ✅ **FIXED**
   - Was: `(ptr, requestID, smHandle, deltaTimeSeconds: Float)`
   - Now: `(ptr, smHandle, deltaTimeNs: Long)` - matches reference
   - JNI binding converts nanoseconds to seconds for internal use

2. **`cppDraw`**
   - Complete parameter reordering needed
   - **Fix:** Align both mprive Kotlin and C++ with reference signature

### P1 - High (Property Operations)

3. **All `cppSet*Property` methods**
   - Remove `requestID` parameter from C++ JNI bindings
   - Match reference fire-and-forget pattern

4. **`cppFireTriggerProperty`**
   - Remove `requestID` parameter from C++ JNI

### P2 - Medium (Asset Operations)

5. **`cppDeleteImage/Audio/Font`**
   - Remove `requestID` from C++

6. **`cppRegisterImage/Audio/Font`**
   - Simplify to match reference `(ptr, name, handle)` signature

7. **`cppUnregisterImage/Audio/Font`**
   - Simplify to match reference `(ptr, name)` signature

---

## Action Items

### Phase 1: Fix Critical Rendering Path

1. Update `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_statemachine.cpp`:
   - Change `cppAdvanceStateMachine` signature to match reference

2. Update `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_rendering.cpp`:
   - Change `cppDraw` signature to match reference

3. Update `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt`:
   - Ensure Kotlin interface matches what C++ expects after fixes

4. Update `mprive/src/androidMain/kotlin/app/rive/mp/core/CommandQueueBridge.android.kt`:
   - Ensure external declarations match

### Phase 2: Fix Property Operations

5. Update all property-related C++ bindings to remove `requestID` parameter

### Phase 3: Fix Asset Operations

6. Update all asset-related C++ bindings to match reference signatures

---

## Files to Modify

### C++ Files
- `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_statemachine.cpp`
- `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_rendering.cpp`
- `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_viewmodel.cpp`
- `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_assets.cpp`

### Kotlin Files
- `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt`
- `mprive/src/androidMain/kotlin/app/rive/mp/core/CommandQueueBridge.android.kt`
- `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt`

### Reference Files (for comparison)
- `kotlin/src/main/kotlin/app/rive/core/CommandQueueBridge.kt`
- `kotlin/src/main/kotlin/app/rive/core/CommandQueue.kt`
- `kotlin/src/main/cpp/bindings_command_queue.cpp`