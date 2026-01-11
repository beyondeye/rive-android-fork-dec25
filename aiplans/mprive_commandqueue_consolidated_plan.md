# mprive CommandQueue Implementation Plan (CONSOLIDATED)

**Date**: January 12, 2026
**Status**: 🔄 Active Development
**Focus**: Android-first approach (Desktop deferred)
**Last Updated**: January 12, 2026

> This plan consolidates the previous `mprive_commandqueue_revised_plan.md` and `mprive_phase0_commandqueue_bridge_plan.md`.
> Archived plans are located in `aiplans/archived/`.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Architecture Overview](#architecture-overview)
3. [Completed Implementation Summary](#completed-implementation-summary)
4. [Remaining Implementation Phases](#remaining-implementation-phases)
5. [Test Fixes Required](#test-fixes-required)
6. [Timeline](#timeline)
7. [Risk Assessment](#risk-assessment)

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
| **Phase D** | View Models & Properties | VMI creation (3 variants), all property types, subscriptions, lists, nested VMI, binding |

**API Coverage**: ~86% of kotlin module features implemented

---

## Remaining Implementation Phases

### Phase C.5: Pointer Events (HIGH PRIORITY)

**Motivation**: User interaction (touch/click/drag) on Rive animations requires pointer event handling. The bridge already has these methods but they're not exposed in `CommandQueue.kt`.

**Status**: 🔴 Not Started

#### Tasks

- [ ] **C.5.1**: Add `pointerMove()` method to CommandQueue
  ```kotlin
  fun pointerMove(smHandle: StateMachineHandle, x: Float, y: Float, pointerID: Int = 0)
  ```

- [ ] **C.5.2**: Add `pointerDown()` method to CommandQueue
  ```kotlin
  fun pointerDown(smHandle: StateMachineHandle, x: Float, y: Float, pointerID: Int = 0)
  ```

- [ ] **C.5.3**: Add `pointerUp()` method to CommandQueue
  ```kotlin
  fun pointerUp(smHandle: StateMachineHandle, x: Float, y: Float, pointerID: Int = 0)
  ```

- [ ] **C.5.4**: Add `pointerExit()` method to CommandQueue
  ```kotlin
  fun pointerExit(smHandle: StateMachineHandle, pointerID: Int = 0)
  ```

- [ ] **C.5.5**: Document coordinate transformation requirements
  - Surface coordinates → artboard coordinates
  - Fit & alignment must be considered

- [ ] **C.5.6**: Write pointer event tests

**Bridge methods already exist**:
- `cppPointerMove()`, `cppPointerDown()`, `cppPointerUp()`, `cppPointerExit()`

---

### Phase E: Advanced Features

**Motivation**: Complete feature parity with kotlin module.

#### E.1: Asset Management

**Status**: 🔴 Not Started

- [ ] **E.1.1**: Image operations
  ```kotlin
  suspend fun decodeImage(bytes: ByteArray): ImageHandle
  fun registerImage(name: String, imageHandle: ImageHandle)
  fun unregisterImage(name: String)
  fun deleteImage(imageHandle: ImageHandle)
  ```

- [ ] **E.1.2**: Audio operations
  ```kotlin
  suspend fun decodeAudio(bytes: ByteArray): AudioHandle
  fun registerAudio(name: String, audioHandle: AudioHandle)
  fun unregisterAudio(name: String)
  fun deleteAudio(audioHandle: AudioHandle)
  ```

- [ ] **E.1.3**: Font operations
  ```kotlin
  suspend fun decodeFont(bytes: ByteArray): FontHandle
  fun registerFont(name: String, fontHandle: FontHandle)
  fun unregisterFont(name: String)
  fun deleteFont(fontHandle: FontHandle)
  ```

- [ ] **E.1.4**: Asset tests

#### E.2: drawToBuffer API

**Status**: 🟡 Bridge Exists, Not Exposed

**Motivation**: Offscreen rendering for screenshots, thumbnails, video export.

- [ ] **E.2.1**: Add `drawToBuffer()` method to CommandQueue
  ```kotlin
  fun drawToBuffer(
      artboardHandle: ArtboardHandle,
      smHandle: StateMachineHandle,
      width: Int,
      height: Int,
      buffer: ByteArray,
      fit: Fit = Fit.CONTAIN,
      alignment: Alignment = Alignment.CENTER,
      clearColor: Int = 0xFF000000.toInt()
  )
  ```

- [ ] **E.2.2**: Write drawToBuffer tests

---

### Phase F: Desktop Support (DEFERRED)

**Motivation**: Kotlin Multiplatform requires desktop support for true cross-platform capability.

**Status**: 🔴 Deferred

- [ ] **F.1**: Create `CommandQueueBridge.desktop.kt` stub
- [ ] **F.2**: Implement Desktop RenderContext (GLFW/Skia)
- [ ] **F.3**: Desktop native bindings (JNA or Kotlin/Native)
- [ ] **F.4**: Cross-platform tests

---

### Phase G: Testing & Optimization

**Motivation**: Production readiness requires comprehensive testing.

**Status**: 🟡 Partial (19/30 tests pass)

#### G.1: Fix Failing Tests

See [Test Fixes Required](#test-fixes-required) section below.

#### G.2: Additional Test Coverage

- [ ] Performance benchmarks (frame times, <16ms budget)
- [ ] Memory leak detection (long-running tests)
- [ ] Stress tests (many files, artboards, high-frequency updates)

---

## Test Fixes Required

**Root Cause**: CommandServer stub limitations, NOT bridge issues. The bridge pattern is working correctly.

### Failing Tests (11 total)

| # | Test | Error | Root Cause | Fix |
|---|------|-------|------------|-----|
| 1 | `MpRiveArtboardLoadTest.queryArtboardNames` | `AssertionError: Expected artboard3` | Stub returns 2 artboards instead of 3 | Update CommandServer stub to return correct artboard count |
| 2 | `MpRiveArtboardLoadTest.queryStateMachineNames` | `IllegalArgumentException: Invalid artboard handle` | Artboard handle expired/invalid | Review artboard handle lifecycle in CommandServer |
| 3 | `MpCommandQueueHandleTest.artboard_handles_are_incrementing` | `AssertionError: handle 2 > handle 1` | Stub doesn't increment handles | Implement incrementing ID generation in stub |
| 4 | `MpCommandQueueHandleTest.handles_remain_valid_across_operations` | `AssertionError: Expected 3 artboards, got 2` | Wrong artboard count | Track file state properly in stub |
| 5 | `MpCommandQueueHandleTest.artboard_handles_are_unique` | `AssertionError: handles should be unique` | Duplicate handles returned | Unique handle generation in stub |
| 6 | `MpRiveDataBindingTest.getBooleanProperty_returnsDefaultValue` | `IllegalArgumentException: Invalid VMI handle` | VMI creation returned invalid handle | Fix VMI creation flow in CommandServer |
| 7 | `MpRiveDataBindingTest.setNestedProperty_via_path` | VMI handle issue | Same as #6 | Same as #6 |
| 8-11 | Various | Similar handle/state issues | Stub state management | Batch fix with stub improvements |

### Fix Strategy

1. **Improve CommandServer stub state management**:
   - Track loaded files with their artboard/SM/VMI counts
   - Generate unique, incrementing handles per resource type
   - Maintain handle validity across operations

2. **Add stub validation**:
   - Validate handles before operations
   - Return proper error messages for invalid handles

---

## Timeline

| Week | Phase | Deliverables |
|------|-------|--------------|
| **Week 1** | C.5 + G.1 | Pointer events API + Fix 11 failing tests |
| **Week 2** | E.1 | Asset management (image, audio, font) |
| **Week 3** | E.2 + G.2 | drawToBuffer API + Performance tests |
| **Future** | F | Desktop support (when prioritized) |

**Total remaining for Android**: ~3 weeks

---

## Risk Assessment

### High Risk

| Risk | Mitigation |
|------|------------|
| **Test Stub Complexity** | Incremental fixes, focus on most critical tests first |
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

- [ ] All 11 failing tests pass
- [ ] Pointer events work with touch/click
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

---

**End of Consolidated Implementation Plan**
