# Sprite Batch Rendering API

This document describes the batch rendering methods available in `CommandQueue` for rendering multiple Rive sprites efficiently.

## Overview

The Rive Android SDK provides two methods for batch rendering multiple sprites:

| Method | Execution | Use Case |
|--------|-----------|----------|
| `drawMultiple()` | **Asynchronous** | TextureView, SurfaceView, real-time display |
| `drawMultipleToBuffer()` | **Synchronous** | Compose Canvas, snapshots, thumbnails |

## drawMultiple() - Asynchronous Rendering

Renders all sprites to a GPU surface asynchronously. The method returns immediately while rendering happens on the command server thread.

### When to Use

- **Real-time display** via TextureView or SurfaceView
- **60fps game loops** where you don't need pixel data
- **Fire-and-forget** rendering scenarios

### Signature

```kotlin
fun drawMultiple(
    commands: List<SpriteDrawCommand>,
    surface: RiveSurface,
    viewportWidth: Int,
    viewportHeight: Int,
    clearColor: Int = Color.TRANSPARENT
)
```

### Example

```kotlin
// Create draw commands from sprites
val commands = sprites.map { sprite ->
    SpriteDrawCommand(
        artboardHandle = sprite.artboard.artboardHandle,
        stateMachineHandle = sprite.stateMachine.stateMachineHandle,
        transform = sprite.computeTransformArray(),
        artboardWidth = sprite.effectiveSize.width,
        artboardHeight = sprite.effectiveSize.height
    )
}

// Render to a TextureView surface (async, non-blocking)
commandQueue.drawMultiple(
    commands = commands,
    surface = textureViewSurface,
    viewportWidth = surface.width,
    viewportHeight = surface.height,
    clearColor = Color.TRANSPARENT
)

// Method returns immediately - rendering happens in background
```

### Threading Model

```
Main Thread                    Command Server Thread
    │                                  │
    ├─── drawMultiple() ──────────────►│
    │    (returns immediately)         │
    │                                  ├─── beginFrame()
    │                                  │
    │    (continue other work)         ├─── render sprites
    │                                  │
    │                                  ├─── flush()
    │                                  │
    │                                  └─── present()
    ▼                                  ▼
```

## drawMultipleToBuffer() - Synchronous Rendering

Renders all sprites and reads pixels back into a byte buffer. The method **blocks** until rendering is complete and pixels are available.

### When to Use

- **Compose Canvas** rendering (via Bitmap)
- **Snapshot/thumbnail** generation
- **Image processing** pipelines
- **Visual regression** testing

### Signature

```kotlin
fun drawMultipleToBuffer(
    commands: List<SpriteDrawCommand>,
    surface: RiveSurface,
    buffer: ByteArray,
    viewportWidth: Int,
    viewportHeight: Int,
    clearColor: Int = Color.TRANSPARENT
)
```

### Buffer Requirements

- **Size**: `width * height * 4` bytes
- **Format**: RGBA (Red, Green, Blue, Alpha)
- **Origin**: Top-left (vertical flip applied automatically)

### Example

```kotlin
// Create draw commands from sprites
val commands = sprites.map { sprite ->
    SpriteDrawCommand(
        artboardHandle = sprite.artboard.artboardHandle,
        stateMachineHandle = sprite.stateMachine.stateMachineHandle,
        transform = sprite.computeTransformArray(),
        artboardWidth = sprite.effectiveSize.width,
        artboardHeight = sprite.effectiveSize.height
    )
}

// Allocate pixel buffer
val pixelBuffer = ByteArray(width * height * 4)

// Render to buffer (sync, blocking ~1-5ms)
commandQueue.drawMultipleToBuffer(
    commands = commands,
    surface = imageSurface,
    buffer = pixelBuffer,
    viewportWidth = width,
    viewportHeight = height,
    clearColor = Color.TRANSPARENT
)

// Buffer now contains RGBA pixel data
// Convert to Bitmap for display
val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
copyRgbaToBitmap(pixelBuffer, bitmap)
```

### Threading Model

```
Main Thread                    Command Server Thread
    │                                  │
    ├─── drawMultipleToBuffer() ──────►│
    │    (blocks)                      │
    │                                  ├─── beginFrame()
    │                                  │
    │    (waiting...)                  ├─── render sprites
    │                                  │
    │                                  ├─── flush()
    │                                  │
    │                                  ├─── glReadPixels()
    │                                  │
    │◄──────────────────── complete ───┤
    │    (unblocks)                    │
    │                                  ▼
    ├─── process pixels
    ▼
```

### Performance Characteristics

| Sprite Count | Typical Latency |
|--------------|-----------------|
| 1-10 sprites | ~1-2ms |
| 10-50 sprites | ~2-3ms |
| 50-100 sprites | ~3-5ms |
| 100+ sprites | ~5-10ms |

These times include:
- GPU rendering
- GPU synchronization (`glFinish()`)
- Pixel readback (`glReadPixels()`)
- Vertical flip for coordinate system conversion

## SpriteDrawCommand

Both methods use `SpriteDrawCommand` to describe each sprite:

```kotlin
data class SpriteDrawCommand(
    val artboardHandle: ArtboardHandle,
    val stateMachineHandle: StateMachineHandle,
    val transform: FloatArray,  // 6-element affine transform
    val artboardWidth: Float,
    val artboardHeight: Float
)
```

### Transform Format

The transform is a 6-element affine transform array:

```
Index | Name       | Matrix Position
------|------------|----------------
  0   | scaleX     | m00
  1   | skewY      | m10
  2   | skewX      | m01
  3   | scaleY     | m11
  4   | translateX | m02
  5   | translateY | m12
```

This corresponds to the matrix:
```
| scaleX  skewX   translateX |
| skewY   scaleY  translateY |
| 0       0       1          |
```

### Creating Commands from RiveSprite

When using the `RiveSpriteScene` API, commands are built automatically:

```kotlin
// RiveSpriteScene handles this internally
val commands = scene.buildDrawCommands()

// Which does:
sprites.filter { it.isVisible }.map { sprite ->
    SpriteDrawCommand(
        artboardHandle = sprite.artboard.artboardHandle,
        stateMachineHandle = sprite.stateMachine.stateMachineHandle,
        transform = sprite.computeTransformArray(),
        artboardWidth = sprite.effectiveSize.width,
        artboardHeight = sprite.effectiveSize.height
    )
}
```

## Best Practices

### 1. Choose the Right Method

| Scenario | Method |
|----------|--------|
| TextureView display | `drawMultiple()` |
| Compose Canvas | `drawMultipleToBuffer()` |
| Game loop (60fps) | `drawMultiple()` |
| Snapshot generation | `drawMultipleToBuffer()` |
| Visual regression tests | `drawMultipleToBuffer()` |

### 2. Reuse Buffers

Allocate pixel buffers once and reuse them:

```kotlin
// ❌ Bad: Allocating new buffer every frame
fun render() {
    val buffer = ByteArray(width * height * 4)  // GC pressure!
    commandQueue.drawMultipleToBuffer(...)
}

// ✅ Good: Reusing buffer
private val pixelBuffer = ByteArray(MAX_WIDTH * MAX_HEIGHT * 4)

fun render() {
    if (sizeChanged) {
        // Only reallocate if needed
        pixelBuffer = ByteArray(width * height * 4)
    }
    commandQueue.drawMultipleToBuffer(..., buffer = pixelBuffer, ...)
}
```

### 3. Pre-sort by Z-Index

Commands are rendered in order, so sort by z-index before calling:

```kotlin
val sortedCommands = commands.sortedBy { it.zIndex }
commandQueue.drawMultiple(sortedCommands, ...)
```

The `RiveSpriteScene.getSortedSprites()` method handles this automatically.

### 4. Handle Errors Gracefully

Both methods can throw exceptions:

```kotlin
try {
    commandQueue.drawMultipleToBuffer(...)
} catch (e: UnsatisfiedLinkError) {
    // Native library not loaded - fall back to per-sprite rendering
    renderPerSprite()
} catch (e: Exception) {
    // Other error - log and fall back
    Log.e(TAG, "Batch render failed", e)
    renderPerSprite()
}
```

## Integration with RiveSpriteScene

The `RiveSpriteScene` API uses `drawMultipleToBuffer()` internally when batch mode is selected:

```kotlin
// Using RiveSpriteScene with batch rendering
Canvas(modifier = Modifier.fillMaxSize()) {
    drawRiveSprites(
        scene = scene,
        renderMode = SpriteRenderMode.BATCH  // Uses drawMultipleToBuffer()
    )
}

// Or per-sprite rendering (default)
Canvas(modifier = Modifier.fillMaxSize()) {
    drawRiveSprites(
        scene = scene,
        renderMode = SpriteRenderMode.PER_SPRITE  // Individual render calls
    )
}
```

## Future: Async Double-Buffered Rendering (Phase 4.5)

For high-performance 60fps game loops that need pixel access, a future phase will add async double-buffered rendering:

```kotlin
// Planned API (not yet implemented)
commandQueue.drawMultipleAsync(
    commands = commands,
    surface = surface,
    buffer = backBuffer,  // Previous frame's buffer
    onComplete = { frontBuffer ->
        // Swap buffers and display
        displayBitmap(frontBuffer)
    }
)
```

This will overlap rendering with display, minimizing latency while maintaining 60fps.
