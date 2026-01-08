# Upstream Merge: rive-app/rive-android (January 2026)

This document details the changes merged from the upstream `rive-app/rive-android` repository into this fork on January 8, 2026.

## Summary

- **15 commits** merged from upstream `rive-app/rive-android` (master branch)
- **30 commits** merged in `rive-runtime` submodule (f3197276 -> 60ddaf6d)
- **1 merge conflict** resolved in `CommandQueue.kt`

## Impact on Kotlin Multiplatform Port

### Critical Changes Requiring Attention

#### 1. CommandQueue Bridge Pattern Refactoring

**Files affected:**
- `kotlin/src/main/kotlin/app/rive/core/CommandQueue.kt` - major refactoring
- `kotlin/src/main/kotlin/app/rive/core/CommandQueueBridge.kt` - **NEW FILE** (912 lines)

**What changed:**
The `CommandQueue` class was refactored to use an abstraction layer (`CommandQueueBridge` interface) for all JNI calls. This allows for mocking in tests.

**Before (direct JNI calls):**
```kotlin
private external fun cppPollMessages(pointer: Long)
private external fun cppLoadFile(pointer: Long, requestID: Long, bytes: ByteArray)
// ... many more external functions
```

**After (bridge pattern):**
```kotlin
class CommandQueue(
    private val renderContext: RenderContext = RenderContextGL(),
    private val bridge: CommandQueueBridge = CommandQueueJNIBridge(),
) : RefCounted {
    // ...
    fun pollMessages() = bridge.cppPollMessages(cppPointer.pointer)
    // ...
}
```

**KMP Port Impact:**
- Your `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` may need similar abstraction
- The bridge pattern could be useful for platform-specific implementations
- Consider implementing a similar `CommandQueueBridge` interface for KMP

#### 2. API Naming Changes

**Renames:**
| Old Name | New Name |
|----------|----------|
| `RiveUI` | `Rive` |
| `RiveUIView` | `RiveView` |
| `rememberCommandQueue` | `rememberRiveWorker` |
| (internal) `CommandQueue` | (alias) `RiveWorker` |

**Files renamed:**
- `kotlin/src/main/kotlin/app/rive/RiveUI.kt` -> `Rive.kt`
- `kotlin/src/main/kotlin/app/rive/RiveUIView.kt` -> `RiveView.kt`
- `kotlin/src/main/kotlin/app/rive/rememberCommandQueue.kt` -> `rememberRiveWorker.kt`

**KMP Port Impact:**
- Your KMP port's public API may want to follow the same naming conventions
- The `RiveWorker` alias is for public use; `CommandQueue` is kept internal

#### 3. New Fit Type System

**New file:** `kotlin/src/main/kotlin/app/rive/Fit.kt` (137 lines)

**What changed:**
The `Fit` type is now more intelligent, containing layout scale factor and alignment only when relevant. Documentation significantly improved.

**KMP Port Impact:**
- Your `mprive/src/commonMain/kotlin/app/rive/mp/core/Fit.kt` may need updates
- Review the new Fit implementation for improved functionality

#### 4. C++ Bindings Changes

**File:** `kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp`

**Changes:**
- All JNI method names now use `CommandQueueJNIBridge` class path instead of `CommandQueue`
- New methods for data binding features
- Improved helper functions in `helpers/general.cpp`

**KMP Port Impact:**
- Your native interop code in `mprive/src/nativeInterop/cpp/` may need similar updates
- The function signatures remain the same, only the JNI class path changed

### New Features

#### 1. Compose Data Binding Support

**New files:**
- `ComposeArtboardBindingActivity.kt` - artboard binding sample
- `ComposeImageBindingActivity.kt` - image binding sample
- `ComposeListActivity.kt` - list binding sample
- `LegacyDataBindingActivity.kt` - legacy API comparison sample

**New methods in `CommandQueue`/`CommandQueueBridge`:**
```kotlin
fun cppReferenceListItemVMI(pointer: Long, requestID: Long, viewModelInstanceHandle: Long, path: String, index: Int): Long
```

**KMP Port Impact:**
- Consider implementing these data binding features in your KMP port
- The `ViewModelInstance.kt` has significant additions for binding support

#### 2. SMI (State Machine Input) Stubs

**Added to this fork as stubs:**
```kotlin
// In CommandQueueBridge.kt - throws UnsupportedOperationException
fun cppSetStateMachineNumberInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Float)
fun cppSetStateMachineBooleanInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Boolean)
fun cppFireStateMachineTrigger(pointer: Long, stateMachineHandle: Long, inputName: String)
```

**Purpose:** These are placeholder methods for RiveSprite support to control animations in legacy Rive files using SMI inputs instead of ViewModels.

**Current status:** Throw `UnsupportedOperationException` - require C++ JNI implementation

**KMP Port Impact:**
- If you need SMI support, implement these in the rive-runtime C++ command_queue/command_server

### rive-runtime Submodule Changes

Updated from `f3197276` to `60ddaf6d` (30 commits).

**Key changes:**

| Category | Description |
|----------|-------------|
| **Bug Fixes** | Solid color change trigger, ScriptedArtboard origin, audio play null checks, path-related crashes |
| **Scripting** | Advance only when active, WASM scripting flag enabled |
| **Rendering** | ClockwiseAtomic shaders moved to new system, skip drawing unchanged frames, Vulkan improvements |
| **Data Binding** | Group effects support, relative data bind paths for all components |
| **Performance** | Vulkan copy optimization for non-input-attachment passes |

**Headers potentially affecting KMP native code:**
- `include/rive/command_queue.hpp` - may have new methods
- `include/rive/command_server.hpp` - may have new implementations

### Files Changed Summary

#### New Files (need review for KMP)
```
kotlin/src/main/kotlin/app/rive/Fit.kt (137 lines)
kotlin/src/main/kotlin/app/rive/core/CommandQueueBridge.kt (912 lines)
kotlin/src/main/kotlin/app/rive/rememberRiveWorker.kt (renamed)
kotlin/src/main/kotlin/app/rive/Rive.kt (renamed)
kotlin/src/main/kotlin/app/rive/RiveView.kt (renamed)
```

#### Modified Files (high impact)
```
kotlin/src/main/kotlin/app/rive/core/CommandQueue.kt - bridge pattern
kotlin/src/main/kotlin/app/rive/ViewModelInstance.kt - data binding additions
kotlin/src/main/kotlin/app/rive/Assets.kt - refactoring
kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp - JNI class path change
```

#### Modified Files (medium impact)
```
kotlin/src/main/kotlin/app/rive/Artboard.kt
kotlin/src/main/kotlin/app/rive/RenderBuffer.kt
kotlin/src/main/kotlin/app/rive/RiveFile.kt
kotlin/src/main/kotlin/app/rive/StateMachine.kt
kotlin/src/main/kotlin/app/rive/core/RenderContext.kt
```

## Conflict Resolution

### CommandQueue.kt

**Conflict type:** Both modified
**Resolution:** Accepted upstream's bridge pattern refactoring, added SMI stub methods

**What was preserved from fork:**
- SMI method signatures (as stubs) for future RiveSprite support

**What was adopted from upstream:**
- CommandQueueBridge interface and pattern
- All API naming changes
- Enhanced documentation

## Recommended Actions for KMP Port

1. **Review CommandQueueBridge pattern** - consider similar abstraction for platform-specific implementations

2. **Update API naming** in KMP port to match:
   - Use `Rive`/`RiveView` instead of `RiveUI`/`RiveUIView`
   - Use `RiveWorker` as public alias for `CommandQueue`

3. **Update Fit implementation** - review new Fit.kt for improved layout handling

4. **Consider data binding support** - ViewModelInstance.kt has significant additions

5. **Update native interop** - JNI class paths changed from `CommandQueue` to `CommandQueueJNIBridge`

6. **Test with new rive-runtime** - submodule updated with bug fixes and improvements

## Testing

After this merge, run the following to verify:
```bash
# Build the project
./gradlew assembleDebug

# Run tests
./gradlew test

# Run Android instrumented tests
./gradlew connectedAndroidTest
```

## Related Commits

### Upstream rive-app/rive-android commits (15):
- `11592921` fix: trigger change on solid color change
- `8f13ae47` chore: advance scripting only if it is still active
- `34629146` chore: Move clockwiseAtomic shaders to new system
- `05ebcf84` test: Add paintType option to player
- `683ef337` fix(runtime): ScriptedArtboard origin fix
- `8de9da34` fix(runtime): additional null checks on audio play
- `1eae8af8` Android/feat databinding demo merge
- `5ab5b784` refactor: Move remaining command queue native functions to bridge
- `4f962146` feat(Android): API alignment refactors
- `6771eb9a` fix(scripting): path-related crashes
- `5b7f014d` chore: Enable with_rive_scripting flag for wasm
- `8ac1f738` feat(Android): Compose remaining data binding
- `7c99351d` chore: complete groups path effects work
- `132baeb8` feature: add group effects support
- `23e84614` Nnnnn relative data bind all paths

---
*Document generated: January 8, 2026*
