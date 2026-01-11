# mprive Phase 0: CommandQueueBridge Refactoring Plan

**Date**: January 11, 2026
**Status**: 🔄 IN PROGRESS
**Priority**: HIGH - Must be completed before Phase E
**Estimated Duration**: 5-7 days
**Last Updated: January 11, 2026, 11:31 PM

---

## Current Implementation Status

### ✅ COMPLETED (Session 1 - Jan 11, 2026)

| Item | Status | File |
|------|--------|------|
| CommandQueueBridge interface | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt` |
| CommandQueueJNIBridge (Android) | ✅ Done | `mprive/src/androidMain/kotlin/app/rive/mp/core/CommandQueueBridge.android.kt` |
| Listeners class | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/core/Listeners.kt` |
| SpriteDrawCommand class | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/core/SpriteDrawCommand.kt` |
| Type aliases (RiveWorker, RivePropertyUpdate) | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| Bridge parameter in CommandQueue constructor | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| expect/actual for createCommandQueueBridge | ✅ Done | Both commonMain and androidMain |

### ✅ COMPLETED (Session 2 - Jan 11, 2026, 12:27 PM)

| Item | Status | File |
|------|--------|------|
| Add `bridge.` prefix to all cppXxx calls | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| Add PropertyUpdate<T> data class | ✅ Done | Inside CommandQueue class (line 66) |
| Android compilation passing | ✅ Done | `./gradlew :mprive:compileDebugKotlinAndroid` succeeds |

### ✅ COMPLETED (Session 3 - Jan 11, 2026, 6:50 PM)

| Item | Status | File |
|------|--------|------|
| Remove all `private external fun` declarations | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| Replace ALL ~40 cppXxx calls with bridge.cppXxx() | ✅ Done | State machine, VMI, property, list, binding ops |
| Fix `advanceStateMachine` signature | ✅ Done | Now uses `deltaTimeNs: Long` instead of `requestID, deltaTimeSeconds: Float` |
| Fix property setter signatures | ✅ Done | Removed `requestID` from setNumber/String/Boolean/Enum/Color/FireTrigger |
| Android compilation passing | ✅ Done | `./gradlew :mprive:compileDebugKotlinAndroid` succeeds |

### ✅ COMPLETED (Session 4 - Jan 11, 2026, 7:02 PM)

| Item | Status | File |
|------|--------|------|
| Convert creation methods from suspend to sync | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| createDefaultArtboard (sync) | ✅ Done | Returns handle directly from bridge |
| createArtboardByName (sync) | ✅ Done | Returns handle directly from bridge |
| createDefaultStateMachine (sync) | ✅ Done | Returns handle directly from bridge |
| createStateMachineByName (sync) | ✅ Done | Returns handle directly from bridge |
| createBlankViewModelInstance (sync) | ✅ Done | Returns handle directly from bridge |
| createDefaultViewModelInstance (sync) | ✅ Done | Returns handle directly from bridge |
| createNamedViewModelInstance (sync) | ✅ Done | Returns handle directly from bridge |
| Android compilation verified | ✅ Done | `./gradlew :mprive:compileDebugKotlinAndroid` succeeds |

### ✅ COMPLETED (Session 5 - Jan 11, 2026, 7:08 PM)

| Item | Status | File |
|------|--------|------|
| Add SMI methods to CommandQueue | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| setStateMachineNumberInput() | ✅ Done | Fire-and-forget wrapper for bridge method |
| setStateMachineBooleanInput() | ✅ Done | Fire-and-forget wrapper for bridge method |
| fireStateMachineTrigger() | ✅ Done | Fire-and-forget wrapper for bridge method |
| Android compilation verified | ✅ Done | `./gradlew :mprive:compileDebugKotlinAndroid` succeeds |

### ✅ COMPLETED (Session 6 - Jan 11, 2026, 11:17 PM)

| Item | Status | File |
|------|--------|------|
| Add batch rendering methods | ✅ Done | `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` |
| drawMultiple() | ✅ Done | Batch sprite rendering with List<SpriteDrawCommand> |
| drawMultipleToBuffer() | ✅ Done | Batch rendering with pixel readback to ByteArray |
| Android compilation verified | ✅ Done | `./gradlew :mprive:compileDebugKotlinAndroid` succeeds |

### ✅ COMPLETED (Session 7 - Jan 11, 2026, 11:31 PM)

| Item | Status | File |
|------|--------|------|
| Update test error message assertions | ✅ Done | `mprive/src/commonTest/kotlin/app/rive/mp/test/statemachine/MpRiveStateMachineLoadTest.kt` |

### 🔄 IN PROGRESS

Phase 0.6 - Test updates complete. Runtime verification requires Android device/emulator.

### ❌ NOT STARTED

| Item | Status | Notes |
|------|--------|-------|
| Add pointer event methods | ❌ Pending | pointerMove, pointerDown, pointerUp, pointerExit |
| Add artboard resize methods | ❌ Pending | resizeArtboard, resetArtboardSize |
| Desktop stub implementation | ❌ Pending | Needed for compilation on desktop target |
| Run instrumented tests on device | ❌ Pending | Requires Android device/emulator |

---

## Next Steps (Continue Here)

1. ~~**Replace external fun declarations in CommandQueue.kt**~~ ✅ DONE:
   - Remove all `private external fun cppXxx(...)` declarations
   - Replace calls like `cppPollMessages(ptr)` with `bridge.cppPollMessages(ptr)`
   - This affects approximately 40+ methods

2. ~~**Convert creation methods to synchronous**~~ ✅ DONE:
   - Changed return type from `suspend fun createDefaultArtboard(): ArtboardHandle` 
   - To: `fun createDefaultArtboard(): ArtboardHandle` (returns handle directly from bridge)
   - All 7 methods converted

3. **Add missing methods** (SMI, pointer events, artboard resize, batch rendering)

4. **Create desktop stub** in `mprive/src/desktopMain/kotlin/app/rive/mp/core/CommandQueueBridge.desktop.kt`

---

## Executive Summary

### Why This Refactoring?

On January 8, 2026, the upstream rive-android repository underwent a significant refactoring that introduced a **Bridge Pattern** for the CommandQueue. This architectural change:

1. **Abstracts JNI calls** behind an interface (`CommandQueueBridge`)
2. **Enables better testing** through mocking
3. **Aligns with multiplatform goals** - each platform can have its own bridge implementation
4. **Changes some APIs** from async to synchronous for creation operations

### Scope of Work

| Category | Changes |
|----------|---------|
| **New Files** | `CommandQueueBridge.kt`, `SpriteDrawCommand.kt`, platform bridge implementations |
| **Modified Files** | `CommandQueue.kt`, C++ bindings |
| **API Changes** | Artboard/SM creation now synchronous, added SMI methods, batch rendering |
| **Type Aliases** | `RiveWorker`, `RivePropertyUpdate` |

---

## Background

### Upstream Changes (rive-app/rive-android)

The `CommandQueue` class was refactored to use an abstraction layer (`CommandQueueBridge` interface) for all JNI calls.

**Before (direct JNI calls):**
```kotlin
private external fun cppPollMessages(pointer: Long)
private external fun cppLoadFile(pointer: Long, requestID: Long, bytes: ByteArray)
```

**After (bridge pattern):**
```kotlin
class CommandQueue(
    private val bridge: CommandQueueBridge = CommandQueueJNIBridge(),
) : RefCounted {
    fun pollMessages() = bridge.cppPollMessages(cppPointer.pointer)
}
```

---

## Implementation Phases

### Phase 0.1: Create CommandQueueBridge Interface (Day 1-2) ✅ DONE

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt`

Create interface with all native method declarations, then create platform-specific implementations:
- `androidMain/kotlin/.../CommandQueueBridge.android.kt` - JNI implementation ✅
- `desktopMain/kotlin/.../CommandQueueBridge.desktop.kt` - JNA/Native implementation (pending)

### Phase 0.2: Update CommandQueue to Use Bridge (Day 2-3) 🔄 ~80% Complete

- ✅ Modify constructor to accept bridge parameter
- ✅ Add bridge. prefix to all native method calls
- ✅ Add PropertyUpdate<T> data class
- ✅ Android compilation passing
- ❌ Convert creation operations from suspend to synchronous
- ❌ Update advanceStateMachine to use Duration

**Methods to convert from suspend to synchronous:**
- createDefaultArtboard
- createArtboardByName
- createDefaultStateMachine
- createStateMachineByName
- createBlankViewModelInstance
- createDefaultViewModelInstance
- createNamedViewModelInstance

### Phase 0.3: Add SMI Methods (Day 3-4) ✅ DONE

Add State Machine Input methods for RiveSprite support:
- `setStateMachineNumberInput` ✅
- `setStateMachineBooleanInput` ✅
- `fireStateMachineTrigger` ✅

### Phase 0.4: Add Batch Rendering (Day 4-5) ✅ DONE

- Add `SpriteDrawCommand` data class ✅ DONE
- Add `drawMultiple()` method ✅ DONE
- Add `drawMultipleToBuffer()` method ✅ DONE

### Phase 0.5: Add Type Aliases (Day 5) ✅ DONE

```kotlin
typealias RiveWorker = CommandQueue
typealias RivePropertyUpdate<T> = CommandQueue.PropertyUpdate<T>
```

### Phase 0.6: Update Tests (Day 5-6) ❌ PENDING

- Update existing tests for synchronous API
- Add bridge mock tests

### Phase 0.7: Update C++ Bindings (Day 6-7) ❌ PENDING

- Update JNI class paths
- Implement new native methods

---

## API Changes Summary

### New Type Aliases
- `RiveWorker` = `CommandQueue`
- `RivePropertyUpdate<T>` = `CommandQueue.PropertyUpdate<T>`

### Methods Changed from Suspend to Synchronous
- createDefaultArtboard
- createArtboardByName
- createDefaultStateMachine
- createStateMachineByName
- createBlankViewModelInstance
- createDefaultViewModelInstance
- createNamedViewModelInstance

### New Methods
- setStateMachineNumberInput (SMI)
- setStateMachineBooleanInput (SMI)
- fireStateMachineTrigger (SMI)
- drawMultiple (batch rendering)
- drawMultipleToBuffer (batch rendering with readback)

---

## Timeline

| Phase | Description | Duration | Status |
|-------|-------------|----------|--------|
| 0.1 | Create CommandQueueBridge interface | Day 1-2 | ✅ Done |
| 0.2 | Update CommandQueue to use bridge | Day 2-3 | ✅ Done |
| 0.3 | Add SMI methods | Day 3-4 | ✅ Done |
| 0.4 | Add batch rendering | Day 4-5 | ✅ Done |
| 0.5 | Add type aliases | Day 5 | ✅ Done |
| 0.6 | Update tests | Day 5-6 | ✅ Done (code review/updates complete, runtime verification pending device) |
| 0.7 | Update C++ bindings | Day 6-7 | ❌ Pending |

**Total: 5-7 days**

---

## Files Created/Modified

### New Files Created
```
mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt
mprive/src/commonMain/kotlin/app/rive/mp/core/Listeners.kt
mprive/src/commonMain/kotlin/app/rive/mp/core/SpriteDrawCommand.kt
mprive/src/androidMain/kotlin/app/rive/mp/core/CommandQueueBridge.android.kt
```

### Files Modified
```
mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt
  - Added imports for bridge classes
  - Added type aliases (RiveWorker, RivePropertyUpdate<T>)
  - Added bridge parameter to constructor
  - ✅ Added bridge. prefix to all native method calls
  - ✅ Added PropertyUpdate<T> data class
```

### Files Pending Creation
```
mprive/src/desktopMain/kotlin/app/rive/mp/core/CommandQueueBridge.desktop.kt (stub)
```

---

## References

- [Upstream Changes Documentation](../docs/merged_upstream_changes_jan_2026.md)
- [Task Description](../aitasks/t1_merge_commandqueue_bridge.md)
- [Original Implementation Plan](mprive_commandqueue_revised_plan.md)

---

**End of Phase 0 Implementation Plan**
