# mprive and mpapp Consolidated Implementation Plan

**Project**: rive-android-fork-dec25  
**Modules**: mprive (Kotlin Multiplatform Library), mpapp (Compose Multiplatform Demo App)  
**Date**: January 18, 2026  
**Status**: Active Development

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current State: mprive Module](#current-state-mprive-module)
3. [Phase 1: Compose API Layer](#phase-1-compose-api-layer)
4. [Phase 2: mpapp Essential Demos](#phase-2-mpapp-essential-demos)
5. [Phase 3: mpapp Additional Demos](#phase-3-mpapp-additional-demos)
6. [Phase 4: Desktop Platform Support](#phase-4-desktop-platform-support)
7. [Milestones & Success Criteria](#milestones--success-criteria)

---

## Executive Summary

### Goal
1. Complete the mprive Compose API layer to enable multiplatform Rive rendering
2. Port essential Compose demos from the `app` module to the `mpapp` module
3. Enable desktop platform support for both mprive and mpapp

### Current Progress
- **mprive (Android)**: ~95% complete - CommandQueue architecture fully implemented
- **mprive (Desktop)**: Stub only - RenderContext and native bindings not implemented
- **mpapp**: Skeleton structure only - basic platform detection

### Key Milestones
1. **M1**: Essential demos working on Android in mpapp
2. **M2**: Desktop platform support in mprive
3. **M3**: Demos working on Desktop in mpapp

---

## Current State: mprive Module

### Architecture Overview

mprive uses a **CommandQueue-based architecture** with a dedicated render thread for thread-safe Rive operations:

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
│  │    - ~70+ native method abstractions                 │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ expect/actual
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Platform Implementation                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Android: CommandQueueJNIBridge        ✅ Complete   │   │
│  │    - JNI external functions                          │   │
│  │    - Links to C++ CommandServer                      │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Desktop: CommandQueueDesktopBridge    ⚠️ Stub Only  │   │
│  │    - Stub implementation (throws NotImplemented)     │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Implemented Features (Android)

| Category | Features | Status |
|----------|----------|--------|
| **File Operations** | `loadFile`, `deleteFile`, `getArtboardNames`, `getViewModelNames`, `getViewModelInstanceNames`, `getViewModelProperties`, `getEnums` | ✅ Complete |
| **Artboard Operations** | `createDefaultArtboard`, `createArtboardByName`, `deleteArtboard`, `resizeArtboard`, `resetArtboardSize` | ✅ Complete |
| **State Machine** | `createDefaultStateMachine`, `createStateMachineByName`, `advanceStateMachine`, `deleteStateMachine`, `getStateMachineNames` | ✅ Complete |
| **SM Inputs** | `setNumberInput`, `setBooleanInput`, `fireTrigger`, `getInputCount`, `getInputNames`, `getInputInfo` | ✅ Complete |
| **Pointer Events** | `pointerMove`, `pointerDown`, `pointerUp`, `pointerExit` | ✅ Complete |
| **VMI Creation** | `createBlankViewModelInstance`, `createDefaultViewModelInstance`, `createNamedViewModelInstance`, `deleteViewModelInstance` | ✅ Complete |
| **VMI Properties** | Get/Set for Number, String, Boolean, Enum, Color + Trigger firing | ✅ Complete |
| **Property Subscriptions** | `subscribeToProperty`, `unsubscribeFromProperty`, property flows | ✅ Complete |
| **List Operations** | `getListSize`, `getListItem`, `addListItem`, `addListItemAt`, `removeListItem`, `removeListItemAt`, `swapListItems` | ✅ Complete |
| **Nested VMI** | `getInstanceProperty`, `setInstanceProperty` | ✅ Complete |
| **Asset Management** | `decodeImage/Audio/Font`, `register/unregister`, `delete` | ✅ Complete |
| **Rendering** | `draw`, `drawMultiple`, `drawToBuffer`, `drawMultipleToBuffer` | ✅ Complete |
| **VMI Binding** | `bindViewModelInstance`, `getDefaultViewModelInstance` | ✅ Complete |

### Missing Features for Compose Integration

| Feature | Description | Priority |
|---------|-------------|----------|
| `Result<T>` pattern | Async loading state wrapper (Loading/Error/Success) | HIGH |
| `rememberRiveWorker()` | Composable for CommandQueue lifecycle | HIGH |
| `rememberRiveFile()` | Composable for file loading with Result | HIGH |
| `rememberArtboard()` | Composable for artboard creation | HIGH |
| `rememberStateMachine()` | Composable for state machine creation | HIGH |
| `rememberViewModelInstance()` | Composable for VMI creation | HIGH |
| `Rive()` composable | Main rendering composable | HIGH |
| `rememberRiveSpriteScene()` | Composable for sprite scene | HIGH |

---

## Phase 1: Compose API Layer

**Priority**: HIGH  
**Prerequisite for**: Phase 2 (mpapp demos)

### 1.1 Result Pattern ✅ COMPLETE

The `Result<T>` sealed interface is already implemented:

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/RiveUI_Result.kt`

```kotlin
sealed interface Result<out T> {
    object Loading : Result<Nothing>
    data class Error(val throwable: Throwable) : Result<Nothing>
    data class Success<T>(val value: T) : Result<T>
    
    // Extension function: andThen
}
```

**Status**: Complete. Additional extension functions (`zip`, `sequence`) can be added as needed.

### 1.2 RiveWorker Composables ✅ COMPLETE

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RememberRiveWorker.kt`

```kotlin
@Composable
@Throws(RiveInitializationException::class)
fun rememberRiveWorker(autoPoll: Boolean = true): CommandQueue

@Composable
fun rememberRiveWorkerOrNull(
    errorState: MutableState<Throwable?> = mutableStateOf(null),
    autoPoll: Boolean = true,
): CommandQueue?
```

**Implementation Details:**
- Uses Compose Multiplatform's `LocalLifecycleOwner` and `repeatOnLifecycle` for lifecycle-aware polling
- Uses Compose's `withFrameNanos` for cross-platform frame timing (works on all platforms)
- Automatically polls `CommandQueue.pollMessages()` every frame while lifecycle is RESUMED
- Properly releases the CommandQueue via `DisposableEffect` when composable leaves scope
- `autoPoll` parameter allows manual control of polling if needed

### 1.3 RiveFile Composables

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RememberRiveFile.kt`

```kotlin
sealed interface RiveFileSource {
    data class Bytes(val bytes: ByteArray) : RiveFileSource
    data class RawRes(val resId: Int, val resources: Resources) : RiveFileSource  // Android only
    data class Path(val path: String) : RiveFileSource  // Desktop/common
}

@ExperimentalRiveComposeAPI
@Composable
fun rememberRiveFile(
    source: RiveFileSource,
    riveWorker: CommandQueue
): Result<RiveFile>
```

### 1.4 Artboard & StateMachine Composables

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RememberArtboard.kt`

```kotlin
@ExperimentalRiveComposeAPI
@Composable
fun rememberArtboard(file: RiveFile, name: String? = null): Artboard

@ExperimentalRiveComposeAPI
@Composable
fun rememberStateMachine(artboard: Artboard, name: String? = null): StateMachine
```

### 1.5 ViewModelInstance Composables

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RememberViewModelInstance.kt`

```kotlin
@ExperimentalRiveComposeAPI
@Composable
fun rememberViewModelInstance(
    file: RiveFile,
    viewModelName: String,
    instanceName: String? = null
): Result<ViewModelInstance>
```

### 1.6 Main Rive Composable

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/compose/Rive.kt`

```kotlin
@ExperimentalRiveComposeAPI
@Composable
fun Rive(
    file: RiveFile,
    modifier: Modifier = Modifier,
    playing: Boolean = true,
    artboard: Artboard? = null,
    stateMachine: StateMachine? = null,
    viewModelInstance: ViewModelInstance? = null,
    fit: Fit = Fit.Contain(),
    backgroundColor: Int = Color.Transparent.toArgb(),
    pointerInputMode: RivePointerInputMode = RivePointerInputMode.Consume,
    onBitmapAvailable: ((getBitmap: GetBitmapFun) -> Unit)? = null
)
```

**Platform-specific implementations:**
- **Android**: Uses `TextureView` + PLS Renderer
- **Desktop**: Uses Compose Canvas + Skia Renderer

### 1.7 Sprite Scene Composables

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RememberRiveSpriteScene.kt`

```kotlin
@ExperimentalRiveComposeAPI
@Composable
fun rememberRiveSpriteScene(riveWorker: CommandQueue): RiveSpriteScene
```

### 1.8 Wrapper Classes

Port or create wrapper classes that encapsulate handle-based APIs:

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/RiveFile.kt`
```kotlin
class RiveFile(
    internal val fileHandle: FileHandle,
    internal val riveWorker: CommandQueue
) {
    // Convenience methods wrapping CommandQueue operations
}
```

Similar wrappers for: `Artboard`, `StateMachine`, `ViewModelInstance`

### 1.9 Lifecycle Management (Compose Multiplatform)

Since JetBrains Compose Multiplatform 1.6.0+, multiplatform lifecycle support is available:

**Dependency**: `org.jetbrains.androidx.lifecycle:lifecycle-runtime-compose:2.8.0`

This provides:
- `LocalLifecycleOwner` - access lifecycle owner from composables
- `Lifecycle.State` - lifecycle states (RESUMED, STARTED, etc.)
- `repeatOnLifecycle` - suspend until lifecycle reaches a state
- `LifecycleEventEffect` - side effects triggered by lifecycle events

This enables the same lifecycle-aware patterns from the Android `kotlin` module to work across all platforms.

### Phase 1 Tasks Checklist

- [x] Create `Result.kt` with sealed interface *(exists in RiveUI_Result.kt)*
- [x] Create `RiveFileSource.kt` with platform-specific implementations *(COMPLETE - 2026-01-18)*
- [x] Create `rememberRiveWorker.kt` composables *(COMPLETE - 2026-01-18)*
- [x] Create `rememberRiveFile.kt` composables *(COMPLETE - 2026-01-18)*
- [x] Create `rememberArtboard.kt` composables *(COMPLETE - 2026-01-18)*
- [x] Create `rememberStateMachine.kt` composables *(COMPLETE - 2026-01-18)*
- [x] Create `rememberViewModelInstance.kt` composables *(COMPLETE - 2026-01-18)*
- [x] Create wrapper classes (RiveFile, Artboard, StateMachine, ViewModelInstance) *(COMPLETE - 2026-01-18)*
- [x] **Create `Rive.kt` main composable** *(COMPLETE - 2026-01-18)* - See Section 1.10 for implementation details
- [ ] Create `rememberRiveSpriteScene.kt` composables - **Deferred: Depends on Rive.kt being tested**
- [ ] Add unit tests for Compose API layer

### 1.10 Rive.kt Main Composable - Detailed Implementation Plan

**Status**: NOT STARTED  
**Complexity**: HIGH (~8-12 hours)  
**Blocking**: Phase 2 demos cannot render without this

#### Overview

The `Rive.kt` composable is the main rendering component that:
1. Creates and manages a rendering surface (TextureView on Android)
2. Handles the animation loop (advance state machine + draw on each frame)
3. Processes pointer input events
4. Binds ViewModelInstances to state machines
5. Handles settled/unsettled states for performance optimization

#### Key Dependencies Already Implemented

| Dependency | Location | Status |
|------------|----------|--------|
| `CommandQueue.draw()` | `CommandQueue.kt` | ✅ Complete |
| `CommandQueue.settledFlow` | `CommandQueue.kt` | ✅ Complete |
| `RenderContextGL.createSurface(surfaceTexture)` | `RenderContext.android.kt` | ✅ Complete |
| `Alignment` enum | `core/Alignment.kt` | ✅ Complete |
| `RiveSurface` | `RiveSurface.kt` | ✅ Complete |
| `Artboard.resizeArtboard()` | `Artboard.kt` | ✅ Complete |
| `Fit` enum | `core/Fit.kt` | ⚠️ Needs conversion to sealed class |

#### Reference Implementation

**File**: `kotlin/src/main/kotlin/app/rive/Rive.kt` (~350 lines)  
**File**: `kotlin/src/main/kotlin/app/rive/Fit.kt` (Fit sealed class)

#### Substeps Checklist

##### 1.10.1 Fit Sealed Class (Match kotlin module API)

**Goal**: Convert from simple enum to sealed class matching kotlin module exactly

**Target API** (from kotlin module):
```kotlin
sealed class Fit {
    open val alignment: Alignment = Alignment.Center
    open val scaleFactor: Float = 1f
    internal abstract val nativeMapping: Byte

    data class Layout(override val scaleFactor: Float = 1f) : Fit()
    data class Contain(override val alignment: Alignment = Alignment.Center) : Fit()
    data class ScaleDown(override val alignment: Alignment = Alignment.Center) : Fit()
    data class Cover(override val alignment: Alignment = Alignment.Center) : Fit()
    data class FitWidth(override val alignment: Alignment = Alignment.Center) : Fit()
    data class FitHeight(override val alignment: Alignment = Alignment.Center) : Fit()
    object Fill : Fit()
    data class None(override val alignment: Alignment = Alignment.Center) : Fit()
}
```

**Substeps:**
- [ ] **1.10.1a**: Create new `Fit.kt` sealed class in `compose/` package
- [ ] **1.10.1b**: Add `nativeMapping: Byte` for JNI compatibility
- [ ] **1.10.1c**: Initial impl WITHOUT alignment (use default `Center`) - alignment support deferred
- [ ] **1.10.1d**: Update `Rive()` composable signature to use sealed class Fit

**Notes on Alignment (Deferred)**:
- Initial implementation uses `Alignment.Center` as default for all fit modes
- Alignment parameter is present in the API but not yet functional
- Full alignment support added in 1.10.1-LATER substeps after basic Rive.kt works

##### 1.10.2 Create RivePointerInputMode Enum
- [ ] Create file: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RivePointerInputMode.kt`
- [ ] Define enum with three modes:
  - `Consume` - Handle and consume pointer events (default)
  - `Observe` - Handle but don't consume (allows parent gestures)
  - `PassThrough` - Handle and share with siblings underneath
- [ ] Add documentation explaining each mode's use case

**Reference**: `kotlin/src/main/kotlin/app/rive/Rive.kt` lines 100-115

##### 1.10.3 Create GetBitmapFun Type Alias
- [ ] Create file or add to existing: `mprive/src/commonMain/kotlin/app/rive/mp/compose/RiveTypes.kt`
- [ ] Add `typealias GetBitmapFun = () -> android.graphics.Bitmap` (Android-specific)
- [ ] Consider if this should be expect/actual for multiplatform

##### 1.10.4 Create Rive.kt Expect Declaration (commonMain)
- [ ] Create file: `mprive/src/commonMain/kotlin/app/rive/mp/compose/Rive.kt`
- [ ] Define expect composable signature:
  ```kotlin
  @ExperimentalRiveComposeAPI
  @Composable
  expect fun Rive(
      file: RiveFile,
      modifier: Modifier = Modifier,
      playing: Boolean = true,
      artboard: Artboard? = null,
      stateMachine: StateMachine? = null,
      viewModelInstance: ViewModelInstance? = null,
      fit: Fit = Fit.Contain(),
      backgroundColor: Int = 0x00000000,
      pointerInputMode: RivePointerInputMode = RivePointerInputMode.Consume,
      onBitmapAvailable: ((GetBitmapFun) -> Unit)? = null
  )
  ```
- [ ] Add comprehensive KDoc documentation

##### 1.10.5 Implement Android Rive.kt (androidMain)
- [ ] Create file: `mprive/src/androidMain/kotlin/app/rive/mp/compose/Rive.android.kt`
- [ ] **Substep 5a**: Set up basic structure with AndroidView + TextureView
  - [ ] Create TextureView factory
  - [ ] Set `isOpaque = false` for transparency support
- [ ] **Substep 5b**: Implement SurfaceTextureListener
  - [ ] `onSurfaceTextureAvailable`: Create RiveSurface via `RenderContextGL.createSurface(surfaceTexture, ...)`
  - [ ] `onSurfaceTextureDestroyed`: Set surface to null, return false (we manage cleanup)
  - [ ] `onSurfaceTextureSizeChanged`: Update width/height state
  - [ ] `onSurfaceTextureUpdated`: Handle bitmap callback if needed
- [ ] **Substep 5c**: Implement DisposableEffect for surface cleanup
  - [ ] Call `riveWorker.destroyRiveSurface(surface)` on dispose
- [ ] **Substep 5d**: Implement artboard fallback with rememberArtboard
  - [ ] If artboard param is null, create default artboard
- [ ] **Substep 5e**: Implement state machine fallback with rememberStateMachine
  - [ ] If stateMachine param is null, create default state machine
- [ ] **Substep 5f**: Implement VMI binding with LaunchedEffect
  - [ ] Subscribe to `viewModelInstance.dirtyFlow` to unsettle on property changes
  - [ ] Call `riveWorker.bindViewModelInstance()` when VMI or SM changes
- [ ] **Substep 5g**: Implement settled flow subscription
  - [ ] Subscribe to `riveWorker.settledFlow.filter { it.handle == stateMachineHandle.handle }`
  - [ ] Set `isSettled = true` when received
- [ ] **Substep 5h**: Implement fit/backgroundColor change handling
  - [ ] LaunchedEffect on fit/backgroundColor to unsettle
- [ ] **Substep 5i**: Implement artboard resize for Fit.Layout
  - [ ] LaunchedEffect on fit + surface dimensions
  - [ ] Call `artboard.resizeArtboard(surface, scaleFactor)` for Layout fit
  - [ ] Call `artboard.resetArtboardSize()` for other fits
- [ ] **Substep 5j**: Implement main drawing loop
  - [ ] LaunchedEffect with keys: lifecycle, surface, handles, fit, playing
  - [ ] Use `repeatOnLifecycle(Lifecycle.State.RESUMED)`
  - [ ] Use `withFrameNanos` for frame timing
  - [ ] Calculate delta time between frames
  - [ ] Skip advance/draw when `isSettled = true`
  - [ ] Call `stateMachine.advance(deltaTime)`
  - [ ] Call `riveWorker.draw(artboardHandle, stateMachineHandle, surface, fit, backgroundColor)`
- [ ] **Substep 5k**: Implement pointer input handling
  - [ ] Create custom `PointerInputModifier` + `PointerInputFilter`
  - [ ] Map Compose pointer events to Rive pointer methods:
    - `PointerEventType.Move` → `pointerMove`
    - `PointerEventType.Press` → `pointerDown`
    - `PointerEventType.Release` → `pointerUp` + `pointerExit`
    - `PointerEventType.Exit` → `pointerExit`
  - [ ] Set `isSettled = false` on any pointer event
  - [ ] Respect `pointerInputMode` for consume/observe/passthrough
  - [ ] Set `shareWithSiblings = true` for PassThrough mode
- [ ] **Substep 5l**: Implement SingleChildLayout wrapper
  - [ ] Simple layout composable that measures single child
  - [ ] Apply pointer input modifier to this wrapper, not AndroidView

**Key considerations**:
- Surface lifecycle must be carefully managed to avoid leaks
- Drawing must only happen when lifecycle is RESUMED
- Settled optimization is important for battery life
- Pointer coordinates must be passed relative to surface dimensions

**Reference files**:
- `kotlin/src/main/kotlin/app/rive/Rive.kt` - Full reference implementation
- `mprive/src/androidMain/kotlin/app/rive/mp/RenderContext.android.kt` - Surface creation

##### 1.10.6 Create Desktop Stub (desktopMain)
- [ ] Create file: `mprive/src/desktopMain/kotlin/app/rive/mp/compose/Rive.desktop.kt`
- [ ] Implement stub that shows a placeholder Box with text "Desktop Rive rendering coming in Phase 4"
- [ ] Log warning about desktop not being implemented

##### 1.10.7 Surface Creation Integration
- [ ] Check if `CommandQueue` needs new method for SurfaceTexture surfaces
- [ ] Current `createRiveSurface(drawKey)` throws on Android
- [ ] Access `RenderContextGL` directly via cast from `renderContext`
- [ ] Alternative: Add `createRiveSurface(surfaceTexture, drawKey)` overload to CommandQueue

**Check**: `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt` - `createRiveSurface` method

##### 1.10.8 Verify Draw Method Signature
- [ ] Check `CommandQueue.draw()` parameters match what Rive.kt needs
- [ ] Ensure it accepts: artboardHandle, stateMachineHandle, surface, fit, backgroundColor
- [ ] Alignment passed through Fit sealed class (deferred - uses Center initially)

**Check**: Search for `fun draw` in CommandQueue.kt

##### 1.10.9 Integration Testing
- [ ] Create simple test in mpapp that uses the new Rive composable
- [ ] Verify file loads and renders
- [ ] Verify touch input works
- [ ] Verify animation plays
- [ ] Verify VMI binding works
- [ ] Test fit modes (Contain, Cover, Fill, Layout)

##### 1.10.1-LATER: Full Alignment Support (DEFERRED)

**When**: After basic Rive.kt works and is tested  
**Why defer**: Alignment support requires verifying native layer, may need C++ changes

- [ ] **L1**: Verify `CommandQueue.draw()` accepts alignment parameter
- [ ] **L2**: Update C++ draw implementation to use alignment (if needed)
- [ ] **L3**: Enable alignment parameter in Fit data classes (remove Center default override)
- [ ] **L4**: Update `Alignment` enum naming to match kotlin module (TopLeft vs TOP_LEFT)
- [ ] **L5**: Visual testing for all fit+alignment combos
- [ ] **L6**: Deprecate/remove old enum `Fit` in `core/Fit.kt`

#### Estimated Effort

| Substep | Effort | Dependencies |
|---------|--------|--------------|
| 1.10.1 Fit sealed class | 1-2 hours | None |
| 1.10.2 PointerInputMode | 30 min | None |
| 1.10.3 GetBitmapFun | 15 min | None |
| 1.10.4 Expect declaration | 30 min | 1.10.1, 1.10.2 |
| 1.10.5 Android actual | 4-6 hours | 1.10.4, 1.10.7, 1.10.8 |
| 1.10.6 Desktop stub | 15 min | 1.10.4 |
| 1.10.7-8 Integration | 1-2 hours | None |
| 1.10.9 Testing | 1-2 hours | All above |
| 1.10.1-LATER Alignment | 2-4 hours | After testing |

**Total**: ~8-12 hours (excluding deferred alignment)

#### Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Surface lifecycle bugs | Follow kotlin module pattern exactly, add logging |
| Pointer event coordinate mapping | Test with simple file first, verify touch hitboxes |
| Memory leaks | Use DisposableEffect for all cleanup, test with profiler |
| Performance issues | Profile drawing loop, ensure settled optimization works |

---

## Phase 2: mpapp Essential Demos

**Priority**: HIGH  
**Depends on**: Phase 1 (Compose API Layer)

### 2.1 Resource Setup

**Task**: Copy all Rive resources from app to mpapp

**Source**: `app/src/main/res/raw/` (~60+ files)  
**Destination**: `mpapp/src/commonMain/resources/rive/`

Create resource loading abstraction:

**File**: `mpapp/src/commonMain/kotlin/demo/resources/DemoResources.kt`
```kotlin
expect object DemoResources {
    fun loadRiveFile(name: String): ByteArray
}
```

**Android**: Load from resources  
**Desktop**: Load from classpath resources

### 2.2 Essential Demo: ComposeActivity

**Original**: `app/src/main/java/app/rive/runtime/example/ComposeActivity.kt`  
**Port to**: `mpapp/src/commonMain/kotlin/demo/ComposeDemo.kt`

**Features to port:**
- Fit mode selection (Contain, Cover, Fill, etc.)
- Alignment selection (TopLeft, Center, BottomRight, etc.)
- Rating widget interaction
- ViewModel data binding

### 2.3 Essential Demo: ComposeDataBindingActivity

**Original**: `app/src/main/java/app/rive/runtime/example/ComposeDataBindingActivity.kt`  
**Port to**: `mpapp/src/commonMain/kotlin/demo/DataBindingDemo.kt`

**Features to port:**
- ViewModel instance creation
- Property get/set operations
- Property change subscriptions
- Nested properties

### 2.4 Essential Demo: ComposeLayoutActivity

**Original**: `app/src/main/java/app/rive/runtime/example/ComposeLayoutActivity.kt`  
**Port to**: `mpapp/src/commonMain/kotlin/demo/LayoutDemo.kt`

**Features to port:**
- Fit.Layout mode
- Responsive artboard resizing
- Surface dimension matching

### 2.5 Essential Demo: SpriteSceneDemoActivity

**Original**: `app/src/main/java/app/rive/runtime/example/sprites/SpriteSceneDemoActivity.kt`  
**Port to**: `mpapp/src/commonMain/kotlin/demo/SpriteSceneDemo.kt`

**Features to port:**
- RiveSpriteScene creation
- Multiple animated sprites
- Position/rotation/scale animation
- Hit testing
- Batch rendering mode toggle
- FPS tracking
- Control panel UI

### 2.6 Demo Navigation

**File**: `mpapp/src/commonMain/kotlin/demo/DemoNavigator.kt`

Create a simple navigation structure to switch between demos:

```kotlin
@Composable
fun DemoApp() {
    var currentDemo by remember { mutableStateOf(Demo.COMPOSE) }
    
    when (currentDemo) {
        Demo.COMPOSE -> ComposeDemo(onNavigate = { currentDemo = it })
        Demo.DATA_BINDING -> DataBindingDemo(onNavigate = { currentDemo = it })
        Demo.LAYOUT -> LayoutDemo(onNavigate = { currentDemo = it })
        Demo.SPRITE_SCENE -> SpriteSceneDemo(onNavigate = { currentDemo = it })
    }
}
```

### Phase 2 Tasks Checklist

- [ ] Copy all resources from `app/src/main/res/raw/` to `mpapp/src/commonMain/resources/rive/`
- [ ] Create `DemoResources.kt` expect/actual for resource loading
- [ ] Port `ComposeActivity` → `ComposeDemo.kt`
- [ ] Port `ComposeDataBindingActivity` → `DataBindingDemo.kt`
- [ ] Port `ComposeLayoutActivity` → `LayoutDemo.kt`
- [ ] Port `SpriteSceneDemoActivity` → `SpriteSceneDemo.kt`
- [ ] Create `DemoNavigator.kt` for demo switching
- [ ] Update `App.kt` to use DemoApp
- [ ] Test all demos on Android emulator/device

---

## Phase 3: mpapp Additional Demos

**Priority**: MEDIUM  
**Depends on**: Phase 2 (Essential demos working)

### Demos to Port (Future)

| Demo | Original File | Port To | Features |
|------|--------------|---------|----------|
| **ComposeListActivity** | `app/.../ComposeListActivity.kt` | `ListDemo.kt` | List properties, add/remove/swap items |
| **ComposeScrollActivity** | `app/.../ComposeScrollActivity.kt` | `ScrollDemo.kt` | Scroll integration, viewport handling |
| **ComposeTouchPassThroughActivity** | `app/.../ComposeTouchPassThroughActivity.kt` | `TouchPassThroughDemo.kt` | Touch event modes (Consume/Observe/PassThrough) |
| **ComposeArtboardBindingActivity** | `app/.../ComposeArtboardBindingActivity.kt` | `ArtboardBindingDemo.kt` | Artboard property binding |
| **ComposeImageBindingActivity** | `app/.../ComposeImageBindingActivity.kt` | `ImageBindingDemo.kt` | Image asset property binding |
| **ComposeAudioActivity** | `app/.../ComposeAudioActivity.kt` | `AudioDemo.kt` | Audio asset playback |
| **LegacyComposeActivity** | `app/.../LegacyComposeActivity.kt` | `LegacyDemo.kt` | Legacy API compatibility |

---

## Phase 4: Desktop Platform Support

**Priority**: MEDIUM (after Android demos work)  
**High-Level Overview**

### 4.1 Desktop RenderContext

Implement a Skia-based RenderContext for Desktop:

**File**: `mprive/src/desktopMain/kotlin/app/rive/mp/RenderContext.desktop.kt`

- Use Skia Canvas from Compose Desktop
- Implement surface creation for off-screen rendering
- Integrate with rive_skia_renderer native library

### 4.2 Desktop CommandQueueBridge

Implement the full CommandQueueBridge for Desktop:

**File**: `mprive/src/desktopMain/kotlin/app/rive/mp/core/CommandQueueBridge.desktop.kt`

- JNI or JNA bindings to desktop native library
- Same API as Android bridge

### 4.3 Desktop Native Library

Build the desktop native library:

**Location**: `mprive/src/desktopMain/cpp/`

- Link against rive_skia_renderer
- Build for Linux x86_64 (primary), macOS, Windows (later)
- Output: `libmprive-desktop.so`

### 4.4 Desktop Rive Composable

Implement desktop-specific Rive composable:

**File**: `mprive/src/desktopMain/kotlin/app/rive/mp/compose/Rive.desktop.kt`

- Use Compose Canvas for rendering
- Integrate with Skia renderer
- Handle pointer events via Compose pointer input

### 4.5 Desktop Testing

- Run all demos on Desktop
- Verify rendering quality matches Android
- Performance profiling

---

## Milestones & Success Criteria

### Milestone 1: Essential Demos on Android (mpapp)

**Target**: 2-3 weeks after Phase 1 complete

**Success Criteria:**
- [ ] Compose API layer (Phase 1) fully implemented
- [ ] All 4 essential demos working on Android in mpapp:
  - [ ] ComposeDemo (fit, alignment, rating)
  - [ ] DataBindingDemo (VMI properties)
  - [ ] LayoutDemo (Fit.Layout)
  - [ ] SpriteSceneDemo (batch rendering, hit testing)
- [ ] No crashes or memory leaks
- [ ] Performance acceptable (60fps target)

### Milestone 2: Desktop Platform Support (mprive)

**Target**: 2-3 weeks after Milestone 1

**Success Criteria:**
- [ ] Desktop RenderContext implemented
- [ ] Desktop CommandQueueBridge implemented
- [ ] Desktop native library builds
- [ ] Basic file loading and rendering works on Desktop
- [ ] At least one demo runs on Desktop

### Milestone 3: Full Desktop Support (mpapp)

**Target**: 1-2 weeks after Milestone 2

**Success Criteria:**
- [ ] All essential demos work on Desktop
- [ ] Rendering quality matches Android
- [ ] Performance acceptable on Desktop (60fps target)
- [ ] Cross-platform code sharing >80%

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Compose API complexity** | Medium | High | Start with minimal API, iterate |
| **Desktop Skia integration** | Medium | High | Research Compose Desktop internals early |
| **Performance on Desktop** | Low | Medium | Profile early, optimize hot paths |
| **Resource loading differences** | Low | Low | Use expect/actual pattern |

---

## References

- **Existing Plans (Archived/Superseded)**:
  - `aiplans/mprive_commandqueue_consolidated_plan.md` - CommandQueue implementation details
  - `aiplans/mprenderer.md` - Multiplatform renderer architecture
  - `aiplans/mprive_android_plus_desktop_plan.md` - Original implementation plan
  - `aiplans/mprive_platform_support.md` - Platform detection and logging
  - `aiplans/mprive_testing_strategy.md` - Testing approach

- **kotlin Module References**:
  - `kotlin/src/main/kotlin/app/rive/Rive.kt` - Main composable reference
  - `kotlin/src/main/kotlin/app/rive/rememberRiveWorker.kt` - Worker composable reference

- **Demo References**:
  - `app/src/main/java/app/rive/runtime/example/` - All Compose demos

---

**End of Consolidated Plan**