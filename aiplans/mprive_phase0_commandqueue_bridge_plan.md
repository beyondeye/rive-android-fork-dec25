# mprive Phase 0: CommandQueueBridge Refactoring Plan

**Date**: January 11, 2026
**Status**: 📋 PLANNED
**Priority**: HIGH - Must be completed before Phase E
**Estimated Duration**: 5-7 days

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

### Phase 0.1: Create CommandQueueBridge Interface (Day 1-2)

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt`

Create interface with all native method declarations, then create platform-specific implementations:
- `androidMain/kotlin/.../CommandQueueBridge.android.kt` - JNI implementation
- `desktopMain/kotlin/.../CommandQueueBridge.desktop.kt` - JNA/Native implementation

### Phase 0.2: Update CommandQueue to Use Bridge (Day 2-3)

- Modify constructor to accept bridge parameter
- Convert creation operations from suspend to synchronous
- Update advanceStateMachine to use Duration

**Methods to convert from suspend to synchronous:**
- createDefaultArtboard
- createArtboardByName
- createDefaultStateMachine
- createStateMachineByName
- createBlankViewModelInstance
- createDefaultViewModelInstance
- createNamedViewModelInstance

### Phase 0.3: Add SMI Methods (Day 3-4)

Add State Machine Input methods for RiveSprite support:
- `setStateMachineNumberInput`
- `setStateMachineBooleanInput`
- `fireStateMachineTrigger`

### Phase 0.4: Add Batch Rendering (Day 4-5)

- Add `SpriteDrawCommand` data class
- Add `drawMultiple()` method (async)
- Add `drawMultipleToBuffer()` method (sync with pixel readback)

### Phase 0.5: Add Type Aliases (Day 5)

```kotlin
typealias RiveWorker = CommandQueue
typealias RivePropertyUpdate<T> = CommandQueue.PropertyUpdate<T>
```

### Phase 0.6: Update Tests (Day 5-6)

- Update existing tests for synchronous API
- Add bridge mock tests

### Phase 0.7: Update C++ Bindings (Day 6-7)

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

| Phase | Description | Duration |
|-------|-------------|----------|
| 0.1 | Create CommandQueueBridge interface | Day 1-2 |
| 0.2 | Update CommandQueue to use bridge | Day 2-3 |
| 0.3 | Add SMI methods | Day 3-4 |
| 0.4 | Add batch rendering | Day 4-5 |
| 0.5 | Add type aliases | Day 5 |
| 0.6 | Update tests | Day 5-6 |
| 0.7 | Update C++ bindings | Day 6-7 |

**Total: 5-7 days**

---

## References

- [Upstream Changes Documentation](../docs/merged_upstream_changes_jan_2026.md)
- [Task Description](../aitasks/t1_merge_commandqueue_bridge.md)
- [Original Implementation Plan](mprive_commandqueue_revised_plan.md)

---

**End of Phase 0 Implementation Plan**
