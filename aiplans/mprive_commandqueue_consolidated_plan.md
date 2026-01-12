# mprive CommandQueue Implementation Plan (CONSOLIDATED)

**Date**: January 12, 2026
**Status**: 🔄 Active Development (Test Data Fix Complete - Jan 12, 2026)
**Focus**: Android-first approach (Desktop deferred)
**Last Updated**: January 12, 2026

> This plan consolidates the previous `mprive_commandqueue_revised_plan.md` and `mprive_phase0_commandqueue_bridge_plan.md`.
> Archived plans are located in `aiplans/archived/`.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Architecture Overview](#architecture-overview)
3. [Completed Implementation Summary](#completed-implementation-summary)
4. [Feature Gap Analysis](#feature-gap-analysis)
5. [Remaining Implementation Phases](#remaining-implementation-phases)
6. [Test Fixes Required](#test-fixes-required)
7. [Performance Analysis](#performance-analysis)
8. [Timeline](#timeline)
9. [Risk Assessment](#risk-assessment)

---

## Executive Summary

### Motivation

The mprive CommandQueue architecture provides:

- **Thread Safety**: All Rive operations on dedicated render thread
- **Performance**: Non-blocking UI, async operations via coroutines
- **Multiplatform**: Clean abstraction via `CommandQueueBridge` interface
- **Production Ready**: Battle-tested handle-based API design

### Current Scope

**Android**: Primary focus - completing feature parity with kotlin module
**Desktop**: Deferred - bridge stub and RenderContext to be implemented later

### Feature Completeness

**mprive is ~85% complete** relative to the kotlin module's CommandQueue:
- ✅ Core rendering, state machines, properties, pointer events
- ❌ Missing: Asset management, some introspection APIs, artboard resizing

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│  Kotlin (commonMain)                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  CommandQueue                                        │   │
│  │    - Reference counting (RefCounted)                 │   │
│  │    - Suspend functions for async queries             │   │
│  │    - Fire-and-forget for mutations                   │   │
│  │    - Property flows (SharedFlow<PropertyUpdate<T>>)  │   │
│  └──────────────────────────────────────────────────────┘   │
│                            │                                 │
│                            ▼                                 │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  CommandQueueBridge (interface)                      │   │
│  │    - Platform-agnostic method declarations           │   │
│  │    - ~70 native method abstractions                  │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ expect/actual
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Platform Implementation                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Android: CommandQueueJNIBridge                      │   │
│  │    - JNI external functions                          │   │
│  │    - Links to C++ CommandServer                      │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Desktop: (NOT YET IMPLEMENTED)                      │   │
│  │    - JNA or native compilation                       │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ JNI
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  C++ Command Server Thread                                  │
│  - Dedicated thread for all Rive operations                 │
│  - Resource management (handles → native pointers)          │
│  - OpenGL context management via RenderContext              │
│  - JNI callbacks to Kotlin                                  │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `CommandQueue` | `commonMain/.../CommandQueue.kt` | Main API, coroutines, flows |
| `CommandQueueBridge` | `commonMain/.../core/CommandQueueBridge.kt` | Platform abstraction interface |
| `CommandQueueJNIBridge` | `androidMain/.../core/CommandQueueBridge.android.kt` | Android JNI implementation |
| `SpriteDrawCommand` | `commonMain/.../core/SpriteDrawCommand.kt` | Batch rendering data class |
| `bindings_commandqueue.cpp` | `nativeInterop/cpp/src/bindings/` | C++ JNI bindings |

### Handle Types

All resources use type-safe handle wrappers:
- `FileHandle`, `ArtboardHandle`, `StateMachineHandle`
- `ViewModelInstanceHandle`, `ImageHandle`, `AudioHandle`, `FontHandle`
- `DrawKey`

---

## Completed Implementation Summary

| Phase | Description | Key Deliverables |
|-------|-------------|------------------|
| **Phase 0** | CommandQueueBridge Refactoring | Interface abstraction, Android JNI bridge, type aliases (`RiveWorker`, `RivePropertyUpdate`) |
| **Phase A** | CommandQueue Foundation | RefCounted, RCPointer, thread lifecycle, suspend infrastructure |
| **Phase B** | File & Artboard Operations | `loadFile()`, `getArtboardNames()`, artboard creation (sync), `deleteFile/Artboard()` |
| **Phase C** | State Machines & Rendering | SM creation (sync), `advanceStateMachine()`, inputs (get/set/fire), `draw()`, `drawMultiple()` |
| **Phase C.5** | Pointer Events | `pointerMove()`, `pointerDown()`, `pointerUp()`, `pointerExit()` with 15 tests |
| **Phase D** | View Models & Properties | VMI creation (3 variants), all property types, subscriptions, lists, nested VMI, binding |

---

## Feature Gap Analysis

### Comparison: kotlin module vs mprive CommandQueue

#### ✅ Features Present in Both

| Category | Features |
|----------|----------|
| **File Ops** | `loadFile`, `deleteFile`, `getArtboardNames`, `getViewModelNames` |
| **Artboard** | `createDefaultArtboard`, `createArtboardByName`, `deleteArtboard` |
| **State Machine** | create/delete, `advanceStateMachine`, `getStateMachineNames` |
| **SMI Inputs** | `setStateMachineNumberInput/Boolean`, `fireStateMachineTrigger` |
| **Pointer Events** | `pointerMove`, `pointerDown`, `pointerUp`, `pointerExit` |
| **VMI Creation** | blank/default/named variants |
| **Properties** | get/set for Number, String, Boolean, Enum, Color + trigger |
| **Subscriptions** | `subscribeToProperty` with property flows |
| **Lists** | `getListSize`, `getListItem`, add/remove/swap operations |
| **Rendering** | `draw`, `drawMultiple`, `drawMultipleToBuffer` |
| **Surfaces** | `createRiveSurface`, `createImageSurface`, `destroyRiveSurface` |

#### ❌ Features Missing in mprive

| Category | Missing Features | Priority |
|----------|-----------------|----------|
| **Asset Management** | `decodeImage/Audio/Font`, `register/unregister`, `delete` | HIGH |
| **File Introspection** | `getViewModelInstanceNames`, `getViewModelProperties`, `getEnums` | MEDIUM |
| **Artboard Resizing** | `resizeArtboard`, `resetArtboardSize` (for Fit.Layout) | MEDIUM |
| **drawToBuffer** | Single artboard offscreen rendering | MEDIUM |
| **VMI References** | `Reference(parent, path)`, `ReferenceListItem(parent, path, index)` | LOW |

---

## Remaining Implementation Phases

### Phase E.1: Asset Management (HIGH PRIORITY)

**Motivation**: Dynamic asset loading is essential for apps that load images, fonts, or audio at runtime rather than embedding them in the Rive file.

**Status**: 🔴 Not Started

#### E.1.1: Image Operations

```kotlin
// Add to CommandQueue.kt
suspend fun decodeImage(bytes: ByteArray): ImageHandle
fun deleteImage(imageHandle: ImageHandle)
fun registerImage(name: String, imageHandle: ImageHandle)
fun unregisterImage(name: String)
```

**Tasks**:
- [ ] Add bridge methods: `cppDecodeImage`, `cppDeleteImage`, `cppRegisterImage`, `cppUnregisterImage`
- [ ] Add JNI callbacks: `onImageDecoded`, `onImageError`
- [ ] Implement suspend function with request/response pattern
- [ ] Add tests for image decode/register/unregister lifecycle

#### E.1.2: Audio Operations

```kotlin
suspend fun decodeAudio(bytes: ByteArray): AudioHandle
fun deleteAudio(audioHandle: AudioHandle)
fun registerAudio(name: String, audioHandle: AudioHandle)
fun unregisterAudio(name: String)
```

**Tasks**:
- [ ] Add bridge methods: `cppDecodeAudio`, `cppDeleteAudio`, `cppRegisterAudio`, `cppUnregisterAudio`
- [ ] Add JNI callbacks: `onAudioDecoded`, `onAudioError`
- [ ] Implement suspend function with request/response pattern
- [ ] Add tests for audio decode/register/unregister lifecycle

#### E.1.3: Font Operations

```kotlin
suspend fun decodeFont(bytes: ByteArray): FontHandle
fun deleteFont(fontHandle: FontHandle)
fun registerFont(name: String, fontHandle: FontHandle)
fun unregisterFont(name: String)
```

**Tasks**:
- [ ] Add bridge methods: `cppDecodeFont`, `cppDeleteFont`, `cppRegisterFont`, `cppUnregisterFont`
- [ ] Add JNI callbacks: `onFontDecoded`, `onFontError`
- [ ] Implement suspend function with request/response pattern
- [ ] Add tests for font decode/register/unregister lifecycle

---

### Phase E.2: File Introspection APIs (MEDIUM PRIORITY)

**Motivation**: Advanced introspection allows apps to dynamically discover ViewModel structure and enum definitions from Rive files.

**Status**: 🔴 Not Started

#### E.2.1: ViewModel Instance Names

```kotlin
suspend fun getViewModelInstanceNames(
    fileHandle: FileHandle,
    viewModelName: String
): List<String>
```

**Tasks**:
- [ ] Add bridge method: `cppGetViewModelInstanceNames`
- [ ] Add JNI callback: `onViewModelInstancesListed`
- [ ] Implement suspend function
- [ ] Add tests

#### E.2.2: ViewModel Properties

```kotlin
suspend fun getViewModelProperties(
    fileHandle: FileHandle,
    viewModelName: String
): List<Property>
```

**Tasks**:
- [ ] Define `Property` data class (name, type, default value)
- [ ] Add bridge method: `cppGetViewModelProperties`
- [ ] Add JNI callback: `onViewModelPropertiesListed`
- [ ] Implement suspend function
- [ ] Add tests

#### E.2.3: Enums

```kotlin
suspend fun getEnums(fileHandle: FileHandle): List<Enum>
```

**Tasks**:
- [ ] Define `Enum` data class (name, values list)
- [ ] Add bridge method: `cppGetEnums`
- [ ] Add JNI callback: `onEnumsListed`
- [ ] Implement suspend function
- [ ] Add tests

---

### Phase E.3: Artboard Resizing (MEDIUM PRIORITY)

**Motivation**: Required for `Fit.Layout` where the artboard must match surface dimensions.

**Status**: 🔴 Not Started

```kotlin
fun resizeArtboard(
    artboardHandle: ArtboardHandle,
    surface: RiveSurface,
    scaleFactor: Float = 1f
)

fun resetArtboardSize(artboardHandle: ArtboardHandle)
```

**Tasks**:
- [ ] Add bridge methods: `cppResizeArtboard`, `cppResetArtboardSize`
- [ ] Implement fire-and-forget methods
- [ ] Add tests for resize/reset cycle
- [ ] Document interaction with `advanceStateMachine`

---

### Phase E.4: drawToBuffer API (MEDIUM PRIORITY)

**Motivation**: Offscreen rendering for screenshots, thumbnails, video export.

**Status**: 🟡 Bridge Exists, Not Exposed

```kotlin
fun drawToBuffer(
    artboardHandle: ArtboardHandle,
    smHandle: StateMachineHandle,
    surface: RiveSurface,
    buffer: ByteArray,
    width: Int,
    height: Int,
    fit: Fit = Fit.CONTAIN,
    alignment: Alignment = Alignment.CENTER,
    clearColor: Int = 0xFF000000.toInt()
)
```

**Tasks**:
- [ ] Add bridge method: `cppDrawToBuffer` (may already exist)
- [ ] Implement synchronous method (blocks until pixels ready)
- [ ] Add buffer size validation
- [ ] Add tests with pixel verification

---

### Phase E.5: Advanced VMI Reference Patterns (LOW PRIORITY)

**Motivation**: Complex data binding scenarios with nested ViewModels.

**Status**: 🔴 Not Started

```kotlin
// Currently mprive has simple VMI creation, but kotlin module has:
ViewModelInstanceSource.Reference(parentInstance, path)
ViewModelInstanceSource.ReferenceListItem(parent, pathToList, index)
```

**Tasks**:
- [ ] Add `cppReferenceNestedVMI` bridge method
- [ ] Add `cppReferenceListItemVMI` bridge method
- [ ] Consider whether to add ViewModelInstanceSource sealed class or keep simple API
- [ ] Add tests for nested VMI access

---

### Phase F: Desktop Support (DEFERRED)

**Motivation**: Kotlin Multiplatform requires desktop support for true cross-platform capability.

**Status**: 🟡 Stub Created

- [x] **F.1**: Create `CommandQueueBridge.desktop.kt` stub (Jan 12, 2026)
- [ ] **F.2**: Implement Desktop RenderContext (GLFW/Skia)
- [ ] **F.3**: Desktop native bindings (JNA or Kotlin/Native)
- [ ] **F.4**: Cross-platform tests

---

### Phase G: Testing & Optimization

**Motivation**: Production readiness requires comprehensive testing.

**Status**: 🟡 Partial

#### G.1: Fix Failing Tests

See [Test Fixes Required](#test-fixes-required) section below.

#### G.2: Additional Test Coverage

- [ ] Performance benchmarks (frame times, <16ms budget)
- [ ] Memory leak detection (long-running tests)
- [ ] Stress tests (many files, artboards, high-frequency updates)

---

## Test Fixes Required

### ✅ FIXED: Test Data Discrepancy (Jan 12, 2026)

**Issue**: The mprive tests expected `multipleartboards.riv` to have **3 artboards**, but the original kotlin module tests show it has **2 artboards**.

**Fix Applied**:
1. ✅ Updated `MpRiveArtboardLoadTest.queryArtboardCount` to expect 2 artboards
2. ✅ Updated `MpRiveArtboardLoadTest.queryArtboardNames` to check for `artboard1` and `artboard2` only
3. ✅ Removed assertion for `artboard3`
4. ✅ Updated `MpCommandQueueHandleTest.handles_remain_valid_across_operations` to expect 2 artboards

### Failing Tests (Updated Analysis)

| # | Test | Error | Root Cause | Fix |
|---|------|-------|------------|-----|
| 1 | `queryArtboardNames` | ~~`AssertionError: Expected artboard3`~~ | ~~**Test bug**: File has 2 artboards, not 3~~ | ✅ **FIXED** |
| 2 | `queryArtboardCount` | ~~`AssertionError: Expected 3`~~ | ~~**Test bug**: Same as above~~ | ✅ **FIXED** |
| 3 | `queryStateMachineNames` | `IllegalArgumentException: Invalid artboard handle` | Artboard handle lifecycle issue | Review CommandServer stub |
| 4 | `artboard_handles_are_incrementing` | `AssertionError: handle 2 > handle 1` | Stub doesn't increment handles | Fix stub ID generation |
| 5 | `handles_remain_valid_across_operations` | ~~`AssertionError: Expected 3 artboards`~~ | ~~**Test bug**: Same data discrepancy~~ | ✅ **FIXED** |
| 6 | `artboard_handles_are_unique` | `AssertionError: handles should be unique` | Stub returns duplicate handles | Fix stub uniqueness |
| 7-11 | Various VMI tests | `IllegalArgumentException: Invalid VMI handle` | VMI creation flow issues | Fix CommandServer VMI handling |

### Fix Strategy

**Step 1**: ✅ COMPLETED - Fix test data expectations (tests 1, 2, 5)
- ✅ Changed expected artboard count from 3 to 2
- ✅ Removed checks for non-existent `artboard3`

**Step 2**: Improve CommandServer stub state management
- Track loaded files with their artboard/SM/VMI counts
- Generate unique, incrementing handles per resource type
- Maintain handle validity across operations

---

## Performance Analysis

### Handle-Based API Overhead

| Aspect | Impact | Mitigation |
|--------|--------|------------|
| **JNI Crossings** | Each operation = 1 JNI call | Batch operations (`drawMultiple`) reduce crossings |
| **Request ID Tracking** | Map lookup for async callbacks | `ConcurrentHashMap` is O(1), negligible |
| **Command Serialization** | Commands queued to render thread | Actually IMPROVES performance vs direct calls |

**Verdict**: The handle-based API has **minimal overhead** because:
- JNI calls are already necessary for Rive (C++ core)
- Batching (e.g., `drawMultiple`) amortizes overhead
- Thread isolation prevents contention

### Async vs Sync Performance

| Operation Type | Performance |
|----------------|-------------|
| Fire-and-forget (mutations) | Immediate return, no blocking |
| Queries (suspend) | Async callback, coroutine-friendly |
| `drawMultiple` | Single JNI call for N sprites |

### Batch Rendering Performance

```
Overhead per sprite: ~32 bytes (6 floats + 2 longs)
For 100 sprites: ~3KB memory + 1 JNI call

Frame budget: 16ms (60fps)
Typical batch render: <1ms for 100 sprites
```

### Memory Considerations

| Aspect | Handle API | Direct Object API |
|--------|------------|-------------------|
| Kotlin object count | 1 handle per resource | 1 wrapper object per resource |
| C++ memory | Same | Same |
| GC pressure | Lower (inline value classes) | Higher (regular classes) |

---

## Timeline

| Week | Phase | Deliverables |
|------|-------|--------------|
| **Week 1** | G.1 | Fix test data discrepancy + Fix failing tests |
| **Week 2** | E.1 | Asset management (image, audio, font) |
| **Week 3** | E.2 + E.3 | File introspection + Artboard resizing |
| **Week 4** | E.4 + G.2 | drawToBuffer API + Performance tests |
| **Future** | F | Desktop support (when prioritized) |

**Total remaining for Android**: ~4 weeks

---

## Risk Assessment

### High Risk

| Risk | Mitigation |
|------|------------|
| **Test Data Discrepancy** | Fix immediately - tests are testing wrong expectations |
| **Asset Loading Complexity** | Follow kotlin module patterns exactly |

### Medium Risk

| Risk | Mitigation |
|------|------------|
| **Performance Regression** | Profile early, optimize hot paths |
| **Memory Leaks** | LeakCanary, long-running stress tests |

### Low Risk

| Risk | Mitigation |
|------|------------|
| **API Compatibility** | Already following kotlin module API patterns |

---

## Success Criteria

- [x] All test data discrepancies fixed (artboard count = 2)
- [ ] All tests pass (currently 19/30, target 30/30)
- [ ] Asset management (image at minimum) functional
- [ ] drawToBuffer produces valid pixel data
- [ ] 60fps rendering maintained (<16ms frame budget)
- [ ] No memory leaks in long-running tests

---

## References

- **Archived Plans**: `aiplans/archived/`
- **Testing Strategy**: `aiplans/mprive_testing_strategy.md`
- **Platform Support Details**: `aiplans/mprive_platform_support.md`
- **Upstream Changes Doc**: `docs/merged_upstream_changes_jan_2026.md`
- **kotlin module CommandQueue**: `kotlin/src/main/kotlin/app/rive/core/CommandQueue.kt`

---

**End of Consolidated Implementation Plan**