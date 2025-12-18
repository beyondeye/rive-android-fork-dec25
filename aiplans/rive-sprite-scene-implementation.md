# RiveSpriteScene Implementation Plan

## Overview

This document outlines the implementation plan for integrating Rive animations with Jetpack Compose Canvas/DrawScope, enabling game engine developers to render multiple Rive animations as game objects with flexible positioning, scaling, rotation, and z-ordering.

---

## Motivation

### Problem Statement

The current `RiveUI` composable is designed for rendering a single Rive animation using a `TextureView` wrapped in `AndroidView`. While this works well for UI elements, it doesn't support the needs of game engines that require:

1. **Multiple independent Rive objects** - Each with its own animation state, position, scale, and rotation
2. **Flexible z-ordering** - Game objects need to be sorted by z-index, not by view hierarchy
3. **DrawScope coordinate integration** - Game engines use Compose Canvas with their own coordinate systems
4. **Per-instance interaction** - Hit testing and animation triggering for each sprite
5. **Performance at scale** - Hundreds of animated objects at 60fps

### Solution: RiveSpriteScene API

We created a new API that:

- Renders multiple Rive artboards in a single GPU pass (batch mode) or individually (per-sprite mode)
- Uses DrawScope coordinates directly (not artboard coordinates)
- Supports flexible z-ordering among sprites
- Provides hit testing and animation control per sprite
- Uses the Rive Renderer (GPU) for maximum performance

---

## Design Rationale

### Why DrawScope Coordinates?

When a developer sets `sprite.position = Offset(100f, 200f)`, the sprite's **origin point** (center or top-left, configurable) should appear at DrawScope coordinate `(100, 200)`. The library internally handles:

1. Scaling the artboard to fit the sprite's requested size
2. Applying the sprite's position, scale, and rotation transforms
3. Converting pointer events back to artboard space for hit testing

This means game code never needs to think about artboard space - everything is in DrawScope units.

### Why Two Rendering Modes?

The implementation provides two rendering approaches to balance compatibility and performance:

**PER_SPRITE Mode (Default):**
- Renders each sprite individually to its cached `RenderBuffer`
- Composites onto a shared bitmap using Android Canvas
- More resilient (one sprite failure doesn't affect others)
- Works without native batch rendering
- Best for debugging or small numbers of sprites (<= 3)

**BATCH Mode:**
- Renders all sprites in a single GPU pass via `drawMultipleToBuffer()`
- Single shared GPU surface for the entire scene
- Much better performance for many sprites
- Lower memory usage
- Best for production use with 5+ sprites. (almost x10 faster for 60 sprites, for example)

### Why Scene-Owned Surface?

The scene owns the shared GPU surface used for batch rendering. This design choice provides simpler lifecycle management (surface tied to scene lifetime).

**Trade-offs:**

| Aspect | Scene-Owned (Current) | Renderer-Owned (Alternative) |
|--------|----------------------|------------------------------|
| Lifecycle | Simple (tied to scene) | Complex (DisposableEffect) |
| Multi-canvas | Surface may not match | Surface always matches |
| API complexity | Simpler | More complex |
| Flexibility | Less flexible | More flexible |

See Phase 4.3 (optional) for migrating to renderer-owned surface if needed.

### Why Synchronous Batch Rendering?

`drawMultipleToBuffer()` is synchronous (blocks ~1-5ms) because:

1. Compose Canvas drawing is already blocking - no benefit to async
2. Simpler API without callbacks or state management
3. Immediate pixel availability for bitmap conversion
4. Acceptable latency for Compose Canvas scenarios

For high-performance 60fps scenarios requiring async rendering, see Phase 4.5 (future enhancement).

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Game Code                                                                  │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  Canvas(modifier) {                                                    │  │
│  │    drawRect(...)           // Background                              │  │
│  │    drawRiveSprites(scene)  // All Rive sprites                        │  │
│  │    drawText(...)           // Foreground UI                           │  │
│  │  }                                                                    │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                           │                                                 │
│                           ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  RiveSpriteScene                                                     │    │
│  │  ├── sprites: SnapshotStateList<RiveSprite>                         │    │
│  │  ├── advance(deltaTime) → advances all state machines               │    │
│  │  ├── getSortedSprites() → z-ordered visible sprites                 │    │
│  │  ├── hitTest(point) → returns topmost hit sprite                    │    │
│  │  ├── buildDrawCommands() → List<SpriteDrawCommand>                  │    │
│  │  └── getOrCreateSharedSurface() → GPU surface for batch mode        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                           │                                                 │
│         ┌─────────────────┼─────────────────┐                               │
│         ▼                 ▼                 ▼                               │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐                │
│  │ RiveSprite      │ │ RiveSprite      │ │ RiveSprite      │                │
│  │ - position      │ │ - position      │ │ - position      │                │
│  │ - scale         │ │ - scale         │ │ - scale         │                │
│  │ - rotation      │ │ - rotation      │ │ - rotation      │                │
│  │ - zIndex        │ │ - zIndex        │ │ - zIndex        │                │
│  │ - origin        │ │ - origin        │ │ - origin        │                │
│  │ - artboard      │ │ - artboard      │ │ - artboard      │                │
│  │ - stateMachine  │ │ - stateMachine  │ │ - stateMachine  │                │
│  │ - renderBuffer  │ │ - renderBuffer  │ │ - renderBuffer  │ (cached)       │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘                │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  CommandQueue (extended)                                             │    │
│  │  ├── drawMultiple(commands, surface) → async, fire-and-forget       │    │
│  │  └── drawMultipleToBuffer(commands, surface, buffer) → sync         │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## File References

### Implemented Files

| File Path | Purpose |
|-----------|---------|
| `kotlin/src/main/kotlin/app/rive/sprites/RiveSprite.kt` | Individual sprite wrapper class |
| `kotlin/src/main/kotlin/app/rive/sprites/RiveSpriteScene.kt` | Scene manager for multiple sprites |
| `kotlin/src/main/kotlin/app/rive/sprites/RiveSpriteSceneRenderer.kt` | Compose integration (composables, extensions) |
| `kotlin/src/main/kotlin/app/rive/sprites/SpriteScale.kt` | Scale data class with independent scaleX/scaleY |
| `kotlin/src/main/kotlin/app/rive/sprites/SpriteOrigin.kt` | Origin sealed class (Center, TopLeft, Custom) |
| `kotlin/src/main/kotlin/app/rive/sprites/SpriteRenderMode.kt` | Enum for PER_SPRITE vs BATCH rendering |

### Modified Files

| File Path | Changes |
|-----------|---------|
| `kotlin/src/main/kotlin/app/rive/core/CommandQueue.kt` | Added `SpriteDrawCommand` data class, `drawMultiple()`, `drawMultipleToBuffer()` methods |
| `kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp` | Added JNI bindings for `cppDrawMultiple`, `cppDrawMultipleToBuffer` |

### Reference Files (for understanding existing patterns)

| File Path | Relevance |
|-----------|-----------|
| `kotlin/src/main/kotlin/app/rive/RiveUI.kt` | Pattern for existing composable |
| `kotlin/src/main/kotlin/app/rive/RiveFile.kt` | File loading pattern |
| `kotlin/src/main/kotlin/app/rive/Artboard.kt` | Artboard wrapper |
| `kotlin/src/main/kotlin/app/rive/StateMachine.kt` | State machine wrapper |
| `kotlin/src/main/kotlin/app/rive/RenderBuffer.kt` | Off-screen rendering pattern |
| `kotlin/src/main/kotlin/app/rive/core/CloseOnce.kt` | Safe disposal pattern |
| `kotlin/src/main/java/app/rive/runtime/kotlin/core/Helpers.kt` | Coordinate conversion utilities |
| `kotlin/src/main/java/app/rive/runtime/kotlin/core/Rive.kt` | `calculateRequiredBounds()` function |

### Test Reference Directory

**Location:** `kotlin/src/androidTest/kotlin/app/rive/`

This directory contains comprehensive tests that serve as reference for:
1. **Writing tests for new features**
2. **Understanding rendering logic and patterns**

| File/Directory | Relevance |
|----------------|-----------|
| `CommandQueueComposeTest.kt` | Tests for CommandQueue with Compose - **directly relevant for sprite scene testing** |
| `snapshot/SnapshotTest.kt` | Snapshot testing patterns for visual regression |
| `snapshot/SnapshotComposeActivity.kt` | Compose-based snapshot testing setup |
| `snapshot/SnapshotBitmapActivity.kt` | Bitmap-based snapshot testing |
| `runtime/kotlin/core/RiveArtboardRendererTest.kt` | Artboard rendering tests - **key reference for rendering logic** |
| `runtime/kotlin/core/RiveStateMachineTouchEventTest.kt` | Touch event handling - **reference for hit testing** |
| `runtime/kotlin/core/RiveMultitouchTest.kt` | Multi-touch handling patterns |
| `runtime/kotlin/core/RiveDataBindingTest.kt` | Data binding tests |
| `runtime/kotlin/core/RiveFileLoadTest.kt` | File loading patterns |
| `runtime/kotlin/core/RiveControllerTest.kt` | Controller tests |
| `runtime/kotlin/core/TestUtils.kt` | Common test utilities - **reusable helpers** |

**Key patterns to follow:**
- Use `runTest` coroutine test scope
- Follow existing screenshot comparison patterns in `snapshot/`
- Use `TestUtils.kt` helpers for common operations
- Mirror the test structure of existing tests

---

## Implementation Checklist

### Phase 1: Core Kotlin Data Structures ✅ COMPLETE

- [x] **1.1 Create `RiveSprite.kt`**
  - [x] Define `RiveSprite` class with artboard and state machine references
  - [x] Add transform properties: `position: Offset`, `scale: SpriteScale`, `rotation: Float`
  - [x] Add `zIndex: Int` for z-ordering
  - [x] Add `size: Size` for the sprite's display size in DrawScope units
  - [x] Add `origin: SpriteOrigin` sealed class (Center, TopLeft, Custom)
  - [x] Add `isVisible: Boolean` flag
  - [x] Implement `fire()`, `setBoolean()`, `setNumber()` (stubbed - TODO comments remain)
  - [x] Implement `advance(deltaTime: Duration)`
  - [x] Implement `computeTransformMatrix(): Matrix`
  - [x] Implement `computeTransformArray(): FloatArray` for native rendering
  - [x] Implement `getArtboardBounds(): Rect` for hit testing
  - [x] Implement `transformPointToLocal(point: Offset): Offset?`
  - [x] Implement `hitTest(point: Offset): Boolean`
  - [x] Implement `getBounds(): Rect` for axis-aligned bounding box
  - [x] Implement cached `RenderBuffer` management (`getOrCreateRenderBuffer()`)
  - [x] Implement `close()` for cleanup

- [x] **1.2 Create `SpriteScale.kt`**
  - [x] Define `SpriteScale` data class with `scaleX`, `scaleY`
  - [x] Add `Unscaled` companion object constant
  - [x] Add `isUniform`, `uniformScale` properties
  - [x] Add operator overloads (`times`, `div`)
  - [x] Add uniform scaling factory function `SpriteScale(scale: Float)`

- [x] **1.3 Create `SpriteOrigin.kt`**
  - [x] Define sealed class with `pivotX`, `pivotY` abstract properties
  - [x] Implement `Center` data object (0.5, 0.5)
  - [x] Implement `TopLeft` data object (0, 0)
  - [x] Implement `Custom` data class with validation (0-1 range)

- [x] **1.4 Create `RiveSpriteScene.kt`**
  - [x] Define `RiveSpriteScene` class
  - [x] Hold reference to `CommandQueue` with acquire/release
  - [x] Manage `SnapshotStateList<RiveSprite>` for Compose state integration
  - [x] Implement `createSprite()`, `addSprite()`, `removeSprite()`, `detachSprite()`, `clearSprites()`
  - [x] Implement `advance(deltaTime: Duration)` to advance all visible state machines
  - [x] Implement `hitTest(point: Offset): RiveSprite?` returning topmost hit
  - [x] Implement `hitTestAll(point: Offset): List<RiveSprite>`
  - [x] Implement `getSortedSprites()`, `getAllSortedSprites()`
  - [x] Implement dirty tracking (`isDirty`, `markDirty()`, `clearDirty()`)
  - [x] Implement shared surface management (`getOrCreateSharedSurface()`, `getPixelBuffer()`)
  - [x] Implement cached composite bitmap (`getOrCreateCompositeBitmap()`)
  - [x] Implement `buildDrawCommands(): List<SpriteDrawCommand>`
  - [x] Implement pointer event routing (`pointerDown()`, `pointerMove()`, `pointerUp()`, `pointerExit()`)
  - [x] Implement `close()` for cleanup using `CloseOnce` pattern

- [x] **1.5 Create `SpriteRenderMode.kt`**
  - [x] Define enum with `PER_SPRITE` and `BATCH` modes
  - [x] Add `DEFAULT` companion constant (currently `PER_SPRITE`)
  - [x] Add KDoc with performance guidance

- [x] **1.6 Create `RiveSpriteSceneRenderer.kt`**
  - [x] Create `rememberRiveSpriteScene(commandQueue: CommandQueue): RiveSpriteScene`
  - [x] Create `DrawScope.drawRiveSprites(scene, renderMode, clearColor)` extension
  - [x] Implement `drawRiveSpritesPerSprite()` for individual sprite rendering
  - [x] Implement `drawRiveSpritesBatch()` for batch GPU rendering
  - [x] Implement `renderSpriteToCanvas()` helper
  - [x] Implement `copyPixelBufferToBitmap()` for BGRA to ARGB conversion
  - [x] Implement `SpriteSceneBuffer` and `SpriteSceneBufferHolder` for caching
  - [x] Add silent fallback from BATCH to PER_SPRITE on errors

### Phase 2: CommandQueue Extensions ✅ COMPLETE

- [x] **2.1 Add `SpriteDrawCommand` data class to `CommandQueue.kt`**
  - [x] Define with `artboardHandle`, `stateMachineHandle`, `transform`, `artboardWidth`, `artboardHeight`
  - [x] Add `init` validation for transform array size (6 elements)
  - [x] Override `equals()`, `hashCode()`, `toString()` for proper FloatArray handling

  ```kotlin
  data class SpriteDrawCommand(
      val artboardHandle: ArtboardHandle,
      val stateMachineHandle: StateMachineHandle,
      val transform: FloatArray,  // 6-element affine transform [a, b, c, d, tx, ty]
      val artboardWidth: Float,
      val artboardHeight: Float,
  )
  ```

- [x] **2.2 Add `drawMultiple()` method (async, fire-and-forget)**
  - [x] Add JNI external declaration `cppDrawMultiple`
  - [x] Implement command flattening into parallel arrays for JNI efficiency
  - [x] Document use case: TextureView/SurfaceView display rendering

- [x] **2.3 Add `drawMultipleToBuffer()` method (sync, with pixel readback)**
  - [x] Add JNI external declaration `cppDrawMultipleToBuffer`
  - [x] Implement command flattening into parallel arrays
  - [x] Document use case: Compose Canvas rendering, snapshots

### Phase 3: Native Implementation ✅ COMPLETE

- [x] **3.1 Add JNI bindings in `bindings_command_queue.cpp`**
  - [x] Implement `Java_app_rive_core_CommandQueue_cppDrawMultiple`
  - [x] Implement `Java_app_rive_core_CommandQueue_cppDrawMultipleToBuffer`
  - [x] Parse Java arrays into C++ vectors for async lambda capture
  - [x] Extract transforms into `rive::Mat2D` format

  **Transform Format:**
  - Uses `rive::Mat2D` which stores 6 floats: `[xx, xy, yx, yy, tx, ty]`
  - Maps to: `[scaleX, skewY, skewX, scaleY, translateX, translateY]`
  - Compatible with Android's `Matrix` values after extraction

- [x] **3.2 Implement batch draw in native code**
  - [x] Iterate through sprites in z-order (pre-sorted on Kotlin side)
  - [x] Get artboard instance from handle (skip if null with warning)
  - [x] Calculate scale to fit artboard into target sprite size
  - [x] Combine transforms: sprite transform * scale-to-fit transform
  - [x] Apply with `renderer.save()` / `renderer.transform()` / artboard `draw()` / `renderer.restore()`
  - [x] Flush GPU render with `riveContext->flush()`
  - [x] Present to surface with `renderContext->present()`

- [x] **3.3 Implement pixel readback for buffer version**
  - [x] Use promise/future pattern for synchronous completion
  - [x] Read pixels with `glReadPixels()`
  - [x] Apply vertical flip (OpenGL origin is bottom-left)

### Phase 4: Compose Canvas Integration ✅ COMPLETE

- [x] **4.1 Implement per-sprite rendering mode**
  - [x] Render each sprite to its cached `RenderBuffer`
  - [x] Composite onto shared bitmap using Android Canvas with transforms
  - [x] Draw composite bitmap to Compose Canvas via `drawImage()`

- [x] **4.2 Implement batch rendering mode**
  - [x] Get or create shared GPU surface from scene
  - [x] Build draw commands from sorted visible sprites
  - [x] Use `drawMultipleToBuffer()` for synchronous rendering
  - [x] Convert pixel buffer (BGRA) to bitmap (ARGB_8888)
  - [x] Draw bitmap to Compose Canvas
  - [x] Silent fallback to per-sprite on `UnsatisfiedLinkError` or exceptions

### Phase 4.3 (Optional): Renderer-Owned Surface

This optional phase migrates surface ownership from the scene to the renderer for better multi-canvas support.

- [ ] **4.3.1 Create SpriteSceneSurface holder class**
  - [ ] Hold RiveSurface reference
  - [ ] Track size for recreation
  - [ ] Implement AutoCloseable

- [ ] **4.3.2 Modify rememberRiveSpriteScene composable**
  - [ ] Add optional surface holder parameter
  - [ ] Default to scene-owned surface if not provided

- [ ] **4.3.3 Create rememberSpriteSceneSurface composable**
  - [ ] Create surface holder with DisposableEffect
  - [ ] Tie to composable lifecycle

### Phase 4.5 (Future): Async Double-Buffered Rendering

For high-performance 60fps scenarios, implement double-buffered async rendering:

- [ ] **4.5.1 Add async readback with callback**
  - [ ] Non-blocking version that signals completion via callback
  - [ ] Double-buffering to overlap render and readback

- [ ] **4.5.2 Integrate with Compose frame timing**
  - [ ] Use previous frame's buffer while current frame renders
  - [ ] Minimize latency while maintaining 60fps

### Phase 5: Hit Testing & Pointer Events ✅ COMPLETE

- [x] **5.1 Implement hit testing in `RiveSprite`**
  - [x] `transformPointToLocal()` - invert sprite transform to get local coordinates
  - [x] `getArtboardBounds()` - return sprite bounds in local space
  - [x] `hitTest(point)` - check if point is within bounds after transform

- [x] **5.2 Implement hit testing in `RiveSpriteScene`**
  - [x] `hitTest(point)` - iterate sprites in reverse z-order, return first hit
  - [x] `hitTestAll(point)` - return all hit sprites, sorted top to bottom

- [x] **5.3 Implement pointer event forwarding**
  
  **RiveSprite methods:**
  - [x] `pointerDown(point, pointerID)` - transform to local, forward to state machine
  - [x] `pointerMove(point, pointerID)` - transform to local, forward to state machine
  - [x] `pointerUp(point, pointerID)` - transform to local, forward to state machine
  - [x] `pointerExit(pointerID)` - send exit event to state machine

  **RiveSpriteScene convenience methods:**
  - [x] `pointerDown(point, pointerID)` - hit test + forward to topmost sprite
  - [x] `pointerMove(point, pointerID, targetSprite?)` - forward to specific or topmost sprite
  - [x] `pointerUp(point, pointerID, targetSprite?)` - forward to specific or topmost sprite
  - [x] `pointerExit(sprite, pointerID)` - send exit event to specific sprite

### Phase 6: Testing & Documentation ⏳ PENDING

- [ ] **6.1 Unit tests**
  - [ ] Test transform computation
  - [ ] Test coordinate conversion (DrawScope → artboard)
  - [ ] Test hit testing with transforms
  - [ ] Test z-order sorting

- [ ] **6.2 Integration tests**
  - [ ] Create sample with multiple sprites
  - [ ] Test performance with 100+ sprites
  - [ ] Test animation triggering

- [ ] **6.3 Documentation**
  - [x] KDoc for all public APIs (implemented inline)
  - [ ] Usage example in README
  - [ ] Performance guidelines

---

## Known Limitations & TODOs

### State Machine Input Methods (RiveSprite)

The following methods are stubbed with TODO comments:
```kotlin
fun fire(triggerName: String) {
    // TODO: Implement when CommandQueue.fireStateMachineTrigger is added
}

fun setBoolean(name: String, value: Boolean) {
    // TODO: Implement when CommandQueue.setStateMachineBoolean is added
}

fun setNumber(name: String, value: Float) {
    // TODO: Implement when CommandQueue.setStateMachineNumber is added
}
```

These require additional CommandQueue extensions to forward inputs to the state machine.

### Artboard Size

`RiveSprite.effectiveSize` currently returns a default size (100x100) when `size` is unspecified because querying artboard dimensions requires native support. The actual artboard dimensions are available in the native layer but not yet exposed through the handle system.

---

## API Examples

### Basic Usage

```kotlin
@Composable
fun GameScreen() {
    val commandQueue = rememberCommandQueue()
    val scene = rememberRiveSpriteScene(commandQueue)
    val enemyFile = rememberRiveFile(R.raw.enemy)
    
    // Create sprites
    val enemies = remember {
        (0 until 50).map { i ->
            scene.createSprite(enemyFile, stateMachineName = "combat").apply {
                position = Offset(
                    x = Random.nextFloat() * 1000f,
                    y = Random.nextFloat() * 800f
                )
                size = Size(64f, 64f)  // 64x64 in DrawScope units
                zIndex = i
            }
        }
    }
    
    // Game loop - advance animations
    LaunchedEffect(Unit) {
        var lastFrameTime = 0L
        while (isActive) {
            withFrameNanos { frameTime ->
                val deltaTime = if (lastFrameTime == 0L) 0L else frameTime - lastFrameTime
                lastFrameTime = frameTime
                scene.advance(deltaTime.nanoseconds)
            }
        }
    }
    
    // Render
    Canvas(modifier = Modifier.fillMaxSize().pointerInput(Unit) {
        detectTapGestures { offset ->
            // Hit test and trigger animation
            scene.hitTest(offset)?.fire("attack")
        }
    }) {
        // Background
        drawRect(Color.DarkGray)
        
        // All Rive sprites in one call
        drawRiveSprites(scene)
        
        // Foreground UI
        drawText(textMeasurer, "Score: 100", topLeft = Offset(10f, 10f))
    }
}
```

### Dynamic Updates

```kotlin
// Move a sprite
enemy.position = Offset(newX, newY)

// Scale a sprite - uniform scaling
enemy.scale = SpriteScale(2.0f)  // 2x size in both dimensions

// Scale a sprite - non-uniform scaling
enemy.scale = SpriteScale(scaleX = 2.0f, scaleY = 1.0f)  // Stretch horizontally
enemy.scale = SpriteScale(scaleX = 1.0f, scaleY = 0.5f)  // Squash vertically

// Reset to original size
enemy.scale = SpriteScale.Unscaled

// Rotate a sprite
enemy.rotation = 45f  // 45 degrees

// Change z-order
enemy.zIndex = 100  // Move to front

// Trigger animation
enemy.fire("hit")

// Change state machine input
enemy.setBoolean("isRunning", true)
enemy.setNumber("speed", 5.0f)

// Remove sprite
scene.removeSprite(enemy)
```

### SpriteOrigin Examples

The `origin` property defines the pivot point for positioning, scaling, and rotation:

```kotlin
// Center origin (default) - sprite.position is the center of the sprite
// Useful for characters, projectiles, rotating objects
sprite.origin = SpriteOrigin.Center

// Top-left origin - sprite.position is the top-left corner
// Useful for UI elements, tiles, grid-based layouts
sprite.origin = SpriteOrigin.TopLeft

// Custom origin - define any pivot point
// (0,0) = top-left, (0.5, 0.5) = center, (1,1) = bottom-right
sprite.origin = SpriteOrigin.Custom(pivotX = 0.5f, pivotY = 1.0f)  // Bottom-center (feet)
sprite.origin = SpriteOrigin.Custom(pivotX = 0.0f, pivotY = 0.5f)  // Left-center
```

**Use cases for Custom origin:**
- **Character sprites**: Set pivot at feet `(0.5, 1.0)` so position represents where the character stands
- **Weapon attachments**: Set pivot at handle point for natural rotation
- **Door hinges**: Set pivot at edge `(0.0, 0.5)` for realistic door opening animations
- **Health bars**: Set pivot at left edge `(0.0, 0.5)` for left-to-right fill animations

### Rendering Modes

```kotlin
// Default: per-sprite rendering (more compatible)
drawRiveSprites(scene)

// Batch rendering (better performance for 20+ sprites)
drawRiveSprites(scene, renderMode = SpriteRenderMode.BATCH)
```

### Pointer Events for Drag

```kotlin
var dragTarget: RiveSprite? = null

Canvas(modifier = Modifier
    .fillMaxSize()
    .pointerInput(Unit) {
        detectDragGestures(
            onDragStart = { offset ->
                dragTarget = scene.pointerDown(offset)
            },
            onDrag = { change, _ ->
                scene.pointerMove(change.position, targetSprite = dragTarget)
            },
            onDragEnd = {
                dragTarget?.let { scene.pointerUp(it.position, targetSprite = it) }
                dragTarget = null
            }
        )
    }
) {
    drawRiveSprites(scene)
}
```

---

## Performance Design Goals (not necessarily reached)

1. **Sprite Limits**: Target 100+ sprites at 60fps on mid-range devices
2. **Rendering Mode Selection**:
   - PER_SPRITE: Best for < 5 sprites, debugging, fallback scenarios
   - BATCH: Best for 5+ sprites, production use
3. **Buffer Caching**: 
   - Per-sprite `RenderBuffer` is cached and reused across frames
   - Composite bitmap is cached in the scene
   - Shared GPU surface is created once per size
4. **Dirty Tracking**: Scene tracks dirty state to skip unnecessary re-renders
5. **Memory Management**:
   - Artboards can be shared across sprites from the same file
   - Each sprite caches its own render buffer (recreated on size change)
   - Composite bitmap is recycled on scene close
6. **Transform Efficiency**: 
   - Transforms computed on Kotlin side, passed as flat arrays to native
   - Native code uses efficient `rive::Mat2D` for GPU rendering

---

## Open Questions (For Future Development)

1. **Texture format**: RGBA_8888 vs RGB_565 for better performance on low-end devices?
2. **Animation batching**: Should we batch `advance()` calls to native code?
3. **Visibility culling**: Should we skip rendering off-screen sprites automatically?
4. **Sprite pooling**: Should we provide sprite pooling for object creation/destruction?
5. **Artboard dimension query**: Should we add native support to query artboard width/height?
