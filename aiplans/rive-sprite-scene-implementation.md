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

We will create a new API that:

- Renders multiple Rive artboards in a single GPU pass
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

### Why Single GPU Surface?

Rendering hundreds of Rive objects to individual surfaces would be prohibitively expensive. Instead:

1. All sprites render to a **single off-screen GPU texture** in one render pass
2. Sprites are sorted by z-index before rendering
3. Per-sprite transforms are applied during the GPU render
4. The final texture is drawn to the Compose Canvas as an image

### Why Not Android Canvas Renderer?

While the Canvas renderer could render directly to a Compose Canvas, the GPU-based Rive Renderer offers:

- Better performance for complex animations
- Hardware acceleration
- Consistent rendering quality
- Support for all Rive features (some require GPU)

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
│  │  ├── sprites: List<RiveSprite>                                      │    │
│  │  ├── advance(deltaTime) → advances all state machines               │    │
│  │  ├── render() → GPU batch render all sprites                        │    │
│  │  └── hitTest(point) → returns topmost hit sprite                    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                           │                                                 │
│         ┌─────────────────┼─────────────────┐                               │
│         ▼                 ▼                 ▼                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                          │
│  │ RiveSprite  │  │ RiveSprite  │  │ RiveSprite  │                          │
│  │ - position  │  │ - position  │  │ - position  │                          │
│  │ - scale     │  │ - scale     │  │ - scale     │                          │
│  │ - rotation  │  │ - rotation  │  │ - rotation  │                          │
│  │ - zIndex    │  │ - zIndex    │  │ - zIndex    │                          │
│  │ - artboard  │  │ - artboard  │  │ - artboard  │                          │
│  │ - stateMach │  │ - stateMach │  │ - stateMach │                          │
│  └─────────────┘  └─────────────┘  └─────────────┘                          │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  CommandQueue (extended)                                             │    │
│  │  └── drawMultiple(commands, surface, clearColor)                    │    │
│  │      - Batches all draw commands                                    │    │
│  │      - Applies per-command transforms on GPU                        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## File References

### Files to Create

| File Path | Purpose |
|-----------|---------|
| `kotlin/src/main/kotlin/app/rive/sprites/RiveSprite.kt` | Individual sprite wrapper class |
| `kotlin/src/main/kotlin/app/rive/sprites/RiveSpriteScene.kt` | Scene manager for multiple sprites |
| `kotlin/src/main/kotlin/app/rive/sprites/RiveSpriteSceneRenderer.kt` | Compose integration (composables, extensions) |
| `kotlin/src/main/kotlin/app/rive/sprites/SpriteScale.kt` | Scale data class with independent scaleX/scaleY |
| `kotlin/src/main/kotlin/app/rive/sprites/SpriteOrigin.kt` | Origin sealed class (Center, TopLeft, Custom) |
| `kotlin/src/main/cpp/include/models/sprite_batch.hpp` | C++ data structures for batch rendering |

### Files to Modify

| File Path | Changes |
|-----------|---------|
| `kotlin/src/main/kotlin/app/rive/core/CommandQueue.kt` | Add `drawMultiple()` method, `SpriteDrawCommand` data class |
| `kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp` | Add JNI bindings for `cppDrawMultiple` |

### Reference Files (unchanged, for understanding)

| File Path | Relevance |
|-----------|-----------|
| `kotlin/src/main/kotlin/app/rive/RiveUI.kt` | Pattern for existing composable |
| `kotlin/src/main/kotlin/app/rive/RiveFile.kt` | File loading pattern |
| `kotlin/src/main/kotlin/app/rive/Artboard.kt` | Artboard wrapper |
| `kotlin/src/main/kotlin/app/rive/StateMachine.kt` | State machine wrapper |
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

### Phase 1: Core Kotlin Data Structures

- [x] **1.1 Create `RiveSprite.kt`** ✅ IMPLEMENTED
  - [x] Define `RiveSprite` class with artboard and state machine references
  - [x] Add transform properties: `position: Offset`, `scale: SpriteScale`, `rotation: Float`
  - [x] Define `SpriteScale` data class (similar to `androidx.compose.ui.layout.ScaleFactor`):
    ```kotlin
    @Immutable
    data class SpriteScale(val scaleX: Float, val scaleY: Float) {
        companion object {
            val Unscaled = SpriteScale(1f, 1f)
        }
    }
    // Extension for uniform scaling
    fun SpriteScale(scale: Float) = SpriteScale(scale, scale)
    ```
  - [x] Add `zIndex: Int` for z-ordering
  - [x] Add `size: Size` for the sprite's display size in DrawScope units
  - [x] Add `origin: SpriteOrigin` sealed class with three types:
    - `SpriteOrigin.Center` - pivot at center (0.5, 0.5)
    - `SpriteOrigin.TopLeft` - pivot at top-left (0, 0)
    - `SpriteOrigin.Custom(pivotX: Float, pivotY: Float)` - custom pivot where (0,0) = top-left, (1,1) = bottom-right
  - [x] Add `isVisible: Boolean` flag
  - [x] Implement `fire(triggerName: String)` for triggering animations (stubbed - needs CommandQueue extension in Phase 2)
  - [x] Implement `setBoolean(name: String, value: Boolean)` for state machine inputs (stubbed - needs CommandQueue extension in Phase 2)
  - [x] Implement `setNumber(name: String, value: Float)` for state machine inputs (stubbed - needs CommandQueue extension in Phase 2)
  - [x] Implement `computeTransformMatrix(): Matrix` to compute the final transform
  - [x] Implement internal `getArtboardBounds(): Rect` for hit testing

- [ ] **1.2 Create `RiveSpriteScene.kt`**
  - [ ] Define `RiveSpriteScene` class
  - [ ] Hold reference to `CommandQueue`
  - [ ] Manage `mutableStateListOf<RiveSprite>()` for sprites
  - [ ] Implement `createSprite(file: RiveFile, artboardName: String?, stateMachineName: String?): RiveSprite`
  - [ ] Implement `removeSprite(sprite: RiveSprite)`
  - [ ] Implement `advance(deltaTime: Duration)` to advance all state machines
  - [ ] Implement `hitTest(point: Offset): RiveSprite?` returning topmost hit
  - [ ] Implement internal `getSortedSprites(): List<RiveSprite>` by z-index
  - [ ] Manage shared GPU surface lifecycle
  - [ ] Implement `close()` for cleanup

- [ ] **1.3 Create `RiveSpriteSceneRenderer.kt`**
  - [ ] Create `rememberRiveSpriteScene(commandQueue: CommandQueue): RiveSpriteScene`
  - [ ] Create `DrawScope.drawRiveSprites(scene: RiveSpriteScene)` extension
  - [ ] Handle surface size changes
  - [ ] Implement automatic texture refresh on animation frame

### Phase 2: CommandQueue Extensions

- [ ] **2.1 Add `SpriteDrawCommand` data class to `CommandQueue.kt`**
  ```kotlin
  data class SpriteDrawCommand(
      val artboardHandle: ArtboardHandle,
      val stateMachineHandle: StateMachineHandle,
      val transform: FloatArray,  // 6-element affine transform [a, b, c, d, tx, ty]
      val artboardWidth: Float,
      val artboardHeight: Float,
  )
  ```

- [ ] **2.2 Add `drawMultiple()` method to `CommandQueue.kt`**
  - [ ] Define method signature:
    ```kotlin
    fun drawMultiple(
        commands: List<SpriteDrawCommand>,
        surface: RiveSurface,
        viewportWidth: Int,
        viewportHeight: Int,
        clearColor: Int = Color.TRANSPARENT
    )
    ```
  - [ ] Add JNI external declaration for `cppDrawMultiple`
  - [ ] Implement command batching logic

### Phase 3: Native Implementation

- [ ] **3.1 Create `sprite_batch.hpp`**
  - [ ] Define C++ struct for sprite draw command:
    ```cpp
    struct SpriteDrawCommand {
        rive::ArtboardHandle artboard;
        rive::StateMachineHandle stateMachine;
        rive::Mat2D transform;
        float artboardWidth;
        float artboardHeight;
    };
    ```

- [ ] **3.2 Add JNI binding in `bindings_command_queue.cpp`**
  - [ ] Implement `Java_app_rive_core_CommandQueue_cppDrawMultiple`
  - [ ] Parse Java array of commands into C++ vector
  - [ ] Extract transform matrix from float array

- [ ] **3.3 Implement batch draw in native code**
  - [ ] Iterate through sorted commands
  - [ ] For each command:
    - [ ] Apply sprite transform with `renderer.save()` / `renderer.transform()` / `renderer.restore()`
    - [ ] Draw artboard at origin
  - [ ] Flush GPU render

### Phase 4: Compose Canvas Integration

- [ ] **4.1 Implement texture management**
  - [ ] Create off-screen GPU surface matching DrawScope size
  - [ ] Handle surface recreation on size change
  - [ ] Implement texture caching for static sprites

- [ ] **4.2 Implement `drawRiveSprites()` extension**
  - [ ] Trigger scene render if dirty
  - [ ] Draw GPU texture to Canvas via `drawImage()`
  - [ ] Handle scaling for different pixel densities

### Phase 5: Hit Testing

- [ ] **5.1 Implement `hitTest()` in `RiveSpriteScene`**
  - [ ] Iterate sprites in reverse z-order (top to bottom)
  - [ ] For each visible sprite:
    - [ ] Invert sprite transform
    - [ ] Transform test point to sprite-local space
    - [ ] Check if point is within artboard bounds
    - [ ] Return first hit

- [ ] **5.2 Implement pointer event forwarding**
  - [ ] Add `RiveSprite.pointerDown(artboardPoint: Offset)`
  - [ ] Add `RiveSprite.pointerMove(artboardPoint: Offset)`
  - [ ] Add `RiveSprite.pointerUp(artboardPoint: Offset)`
  - [ ] Forward to state machine for interactive elements

### Phase 6: Testing & Documentation

- [ ] **6.1 Unit tests**
  - [ ] Test transform computation
  - [ ] Test coordinate conversion (DrawScope → artboard)
  - [ ] Test hit testing with transforms
  - [ ] Test z-order sorting

- [ ] **6.2 Integration tests**
  - [ ] Create sample game with multiple sprites
  - [ ] Test performance with 100+ sprites
  - [ ] Test animation triggering

- [ ] **6.3 Documentation**
  - [ ] KDoc for all public APIs
  - [ ] Usage example in README
  - [ ] Performance guidelines

---

## API Examples

### Basic Usage

```kotlin
@Composable
fun GameScreen() {
    val scene = rememberRiveSpriteScene()
    val enemyFile = rememberRiveFile(R.raw.enemy)
    
    // Create sprites
    val enemies = remember {
        (0 until 50).map { i ->
            scene.createSprite(enemyFile, stateMachine = "combat").apply {
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
sprite.origin = SpriteOrigin.Custom(pivotX = 0.5f, pivotY = 1.0f)  // Bottom-center (for feet-based positioning)
sprite.origin = SpriteOrigin.Custom(pivotX = 0.0f, pivotY = 0.5f)  // Left-center (for side-scrolling alignment)
sprite.origin = SpriteOrigin.Custom(pivotX = 0.25f, pivotY = 0.75f)  // Any arbitrary pivot point
```

**Use cases for Custom origin:**
- **Character sprites**: Set pivot at feet `(0.5, 1.0)` so position represents where the character stands
- **Weapon attachments**: Set pivot at handle point for natural rotation
- **Door hinges**: Set pivot at edge `(0.0, 0.5)` for realistic door opening animations
- **Health bars**: Set pivot at left edge `(0.0, 0.5)` for left-to-right fill animations

---

## Performance Considerations

1. **Sprite Limits**: Target 200+ sprites at 60fps on mid-range devices
2. **Batching**: All sprites render in a single GPU draw call per frame
3. **Dirty Tracking**: Only re-render if sprites changed or animations advanced
4. **Memory**: Artboards shared across sprites from the same file
5. **Texture Size**: Match DrawScope size, recreate on resize

---

## Open Questions (To Resolve During Implementation)

1. **Texture format**: RGBA_8888 vs RGB_565 for better performance?
2. **Animation batching**: Should we batch `advance()` calls to native code?
3. **Visibility culling**: Should we skip rendering off-screen sprites?
4. **Sprite pooling**: Should we provide sprite pooling for object creation/destruction?

---

## Timeline Estimate

| Phase | Estimated Duration |
|-------|-------------------|
| Phase 1: Kotlin structures | 2-3 days |
| Phase 2: CommandQueue extensions | 1 day |
| Phase 3: Native implementation | 3-4 days |
| Phase 4: Compose integration | 2 days |
| Phase 5: Hit testing | 1-2 days |
| Phase 6: Testing & docs | 2-3 days |
| **Total** | **11-15 days** |

---

## Contribution Guidelines

This implementation is designed to be contributed back to `rive-android`. Please ensure:

1. All code follows existing project conventions
2. KDoc comments on all public APIs
3. Unit tests for new functionality
4. Integration with existing CI/CD pipeline
5. Backward compatibility with existing `RiveUI` API
