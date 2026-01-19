# Task: Solve Grey Box Rendering Issue in mprive RiveDemo

**Date**: January 19, 2026
**Status**: ✅ SOLVED
**Related**: t3_crash_when_running_demoapp.md

---

## Problem Statement

When running `RiveDemo.kt` in the mpapp module on Android, the screen displays only a grey box instead of the expected Rive animation content. The logs indicate that rendering commands complete "successfully" but no visual output is displayed.

---

## Investigation Summary

### Evidence Gathered

#### 1. Log Analysis (from t3_crash_when_running_demoapp.md)

Key observations from the logs:
- CommandServer worker thread starts successfully
- RenderContext initializes successfully
- File loads successfully
- Artboard created (500x500 dimensions)
- State machine created
- Render target created (984x1332, **sample count: 0**)
- Draw command enqueues and completes with "success" message
- **No error messages** - but grey box displayed

```
CommandServer: RenderContext initialized successfully
CommandServer: Handling LoadFile command
CommandServer: File loaded successfully, handle = 1
CommandServer: Handling CreateDefaultArtboard
CommandServer: Handling CreateDefaultStateMachine
Creating Rive render target on command server thread (984x1332, sample count: 0)
CommandServer: Handling Draw command (artboard=500x500)
CommandServer: Draw command completed successfully
```

#### 2. Architecture Comparison

**Original rive-android** (`kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp`):
- Uses Rive's built-in `rive::CommandQueue` and `rive::CommandServer`
- Draw uses lambda passed to `commandQueue->draw(drawKey, drawWork)`
- Lambda receives `rive::CommandServer*` parameter for factory access
- Uses `CommandServerFactory` pattern for render context access

**mprive** (`mprive/src/nativeInterop/cpp/`):
- Custom `CommandServer` implementation
- Draw enqueues a `Command` struct processed by `handleDraw()`
- Uses `m_renderContext` stored during construction
- No factory pattern - direct member access

#### 3. Code Flow Analysis

**mprive Draw Sequence** (in `command_server_render.cpp::handleDraw`):
```cpp
1. Validate artboard handle → Found in m_artboards map
2. Validate state machine handle → Found in m_stateMachines map  
3. Check m_renderContext → Not null
4. Check riveContext → Not null
5. Check renderTarget → Not null (from cmd.renderTargetPtr)
6. renderContext->beginFrame(surfacePtr)  // Make EGL surface current
7. riveContext->beginFrame(...)           // Start Rive GPU frame
8. Create RiveRenderer and draw artboard
9. riveContext->flush({.renderTarget})    // Flush to render target
10. renderContext->present(surfacePtr)    // Swap EGL buffers
11. Send success message
```

This sequence MATCHES the original rive-android implementation.

#### 4. RenderContext Implementation

Both implementations use nearly identical `RenderContextGL` classes:
- Same PBuffer surface creation for initial context binding
- Same `beginFrame()` using `eglMakeCurrent`
- Same `present()` using `eglSwapBuffers`

#### 5. runOnce Implementation

mprive has a working `runOnce` implementation:
```cpp
void CommandServer::runOnce(std::function<void()> func) {
    auto promise = std::make_shared<std::promise<void>>();
    std::future<void> future = promise->get_future();
    
    Command cmd(CommandType::RunOnce);
    cmd.runOnceCallback = [func = std::move(func), promise]() {
        func();
        promise->set_value();
    };
    
    enqueueCommand(std::move(cmd));
    future.wait();  // Blocks until executed on worker thread
}
```

Used correctly in `cppCreateRiveRenderTarget` for synchronous render target creation.

---

## Potential Root Causes

### Hypothesis 1: EGL Surface/Context Issues
- `eglMakeCurrent` might be failing silently
- `eglSwapBuffers` might not be presenting to the correct surface
- The surface pointer from Kotlin might not match what TextureView expects

### Hypothesis 2: Render Target Creation Timing
- Render target created with sample count = 0 (unusual)
- Created when PBuffer is current, not window surface
- FBO 0 reference might not correctly point to window framebuffer

### Hypothesis 3: Artboard Content
- Artboard dimensions (500x500) differ from surface (984x1332)
- Fit/alignment transformation might be incorrect
- Content might be rendering but outside visible bounds

### Hypothesis 4: Draw Command Parameters
- `renderContextPtr` passed as `requestID` (unusual pattern)
- Surface pointer chain of casts: jlong → int64_t → void* → EGLSurface

---

## Files Examined

| File | Purpose | Notes |
|------|---------|-------|
| `kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp` | Original draw implementation | Uses lambda-based draw pattern |
| `kotlin/src/main/cpp/include/models/render_context.hpp` | Original RenderContextGL | Template for mprive version |
| `mprive/src/nativeInterop/cpp/src/command_server/command_server_render.cpp` | mprive handleDraw | Same sequence as original |
| `mprive/src/nativeInterop/cpp/include/render_context.hpp` | mprive RenderContextGL | Near-identical to original |
| `mprive/src/nativeInterop/cpp/src/command_server/command_server_core.cpp` | Worker thread + runOnce | Correct implementation |
| `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_rendering.cpp` | JNI draw binding | Uses runOnce for render target |

---

## Options Going Forward

### Option A: Fix Draw to Use runOnce Pattern (Structural Fix)
Change `cppDraw` to use `runOnce` with full draw logic inside:
```cpp
server->runOnce([all params]() {
    // Complete draw sequence in one synchronous block
});
```

**Pros**: Matches original pattern more closely
**Cons**: Changes async draw to sync, may affect performance

### Option B: Add Diagnostic Logging First (Current Approach)
Add detailed logging to trace:
- EGL operation return values
- Surface pointer values at each step
- GL state before/after operations

**Pros**: Pinpoints exact failure location
**Cons**: Requires rebuild and test cycle

### Option C: Verify Surface Creation in Kotlin
Check how RiveSurface acquires and passes surface pointers:
- Verify TextureView → SurfaceTexture → Surface chain
- Confirm EGL surface creation from native window

**Pros**: May find Kotlin-side issue
**Cons**: Requires understanding Kotlin surface management

---

## Next Steps

1. **Add Diagnostic Logging** (Current)
   - Log EGL return values in beginFrame/present
   - Log surface pointer values
   - Log GL errors after operations
   - Verify render target properties

2. **Run and Analyze Logs**
   - Look for EGL errors
   - Check if surface pointers match
   - Verify GL state

3. **Implement Fix Based on Findings**
   - If EGL issue: Fix surface handling
   - If timing issue: Use runOnce pattern
   - If Kotlin issue: Fix surface creation

---

## Diagnostic Logging Plan

### Files Modified (January 19, 2026)

1. **`mprive/src/nativeInterop/cpp/include/render_context.hpp`** ✅
   - Added surface pointer logging to `beginFrame()`
   - Added EGL display/context logging
   - Added `eglMakeCurrent` return value verification
   - Added current context/surface verification after bind
   - Added surface pointer logging to `present()`
   - Added `eglSwapBuffers` return value verification

2. **`mprive/src/nativeInterop/cpp/src/command_server/command_server_render.cpp`** ✅
   - Added `<GLES3/gl3.h>` include for GL error checking
   - Added draw parameter logging (surfacePtr, renderTargetPtr, drawKey, fit, alignment, etc.)
   - Added surface pointer logging before `beginFrame`
   - Added GL error checks after each major operation:
     - After `beginFrame`
     - After `riveContext->beginFrame`
     - After `artboard->draw`
     - After `riveContext->flush`
     - After `present`
   - Added render target dimension logging
   - Added current FBO binding check after flush
   - Added artboard name and dimension logging

3. **`mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_rendering.cpp`** ✅
   - Added `<EGL/egl.h>` include
   - Added current EGL context/surface logging during render target creation
   - Added GL error checking after sample count query
   - Added GL viewport dimension query and logging
   - Added created render target pointer logging

---

## Expected Log Output

After running with the new logging, look for these key patterns in logcat:

```
# Filter: mprive|CommandServer|RenderContext

# During render target creation:
CreateRiveRenderTarget: current EGL context=0xXXX, draw surface=0xXXX
CreateRiveRenderTarget: current GL viewport: x=0, y=0, w=X, h=Y
Creating Rive render target on command server thread (WxH, sample count: N)
CreateRiveRenderTarget: created renderTarget=0xXXX

# During draw:
CommandServer: Draw params - surfacePtr=XXX, renderTargetPtr=XXX
[RenderContext] beginFrame: surface ptr=0xXXX, eglSurface=0xXXX
[RenderContext] beginFrame: eglMakeCurrent succeeded
CommandServer: Calling riveContext->beginFrame(WxH, clearColor=0xXXXXXXXX)
CommandServer: Drawing artboard 'name' (WxH)
CommandServer: Flushing to renderTarget=0xXXX (width=W, height=H)
CommandServer: After flush, current FBO binding=0
[RenderContext] present: eglSwapBuffers succeeded
CommandServer: Draw command completed successfully
```

### What to Look For

| Log Pattern | What It Means |
|-------------|---------------|
| `eglMakeCurrent succeeded` | EGL surface bind working |
| `GL error after X: 0xYYYY` | GL error occurred (bad sign) |
| `eglSwapBuffers succeeded` | Buffer swap working |
| `current FBO binding=0` | Rendering to default framebuffer |
| Surface pointers matching | Same surface used throughout |

---

## SOLUTION IMPLEMENTED (January 19, 2026)

### Root Cause

The grey box was caused by **using `NoOpFactory` instead of the GPU render context's factory** when loading Rive files.

In `command_server_file.cpp::handleLoadFile()`, there was a TODO comment that was never completed:

```cpp
if (m_renderContext != nullptr) {
    // TODO: Get factory from render context in Phase C  ← NEVER COMPLETED!
    // factory = static_cast<RenderContext*>(m_renderContext)->getFactory();
}
if (factory == nullptr) {
    // ALWAYS fell through to here!
    factory = m_noOpFactory.get();
}
```

**What `NoOpFactory` does**: Creates dummy/no-op render objects (paths, paints, images) that don't actually render anything. The file loads correctly, artboards and state machines work, but all visual content becomes invisible!

### Fix Applied

**1. Added `getFactory()` method to `RenderContext` base class** (`render_context.hpp`)

```cpp
virtual rive::Factory* getFactory() const
{
    return riveContext ? riveContext.get() : nullptr;
}
```

This is multiplatform-ready:
- `rive::gpu::RenderContext` (stored as `riveContext`) inherits from `rive::Factory`
- Works on all platforms (Android, Desktop, iOS, WASM)
- Virtual method allows platform-specific overrides if needed (e.g., custom image decoding)

**2. Updated `handleLoadFile` to use `getFactory()`** (`command_server_file.cpp`)

```cpp
rive::Factory* factory = nullptr;
if (m_renderContext != nullptr) {
    auto* renderContext = static_cast<rive_mp::RenderContext*>(m_renderContext);
    factory = renderContext->getFactory();
    if (factory != nullptr) {
        LOGI("CommandServer: Using GPU factory from RenderContext");
    }
}
if (factory == nullptr) {
    LOGW("CommandServer: No GPU factory available, using NoOpFactory (content will not render)");
    // ...fallback to NoOpFactory
}
```

### Why This Works

1. `rive::gpu::RenderContext` inherits from `rive::Factory`
2. When Rive imports a file, it uses the factory to create render objects
3. With `NoOpFactory`: Creates no-op objects → nothing renders
4. With `riveContext`: Creates GPU-accelerated render objects → proper rendering

### Files Modified

| File | Changes |
|------|---------|
| `mprive/src/nativeInterop/cpp/include/render_context.hpp` | Added `getFactory()` method with comprehensive multiplatform documentation |
| `mprive/src/nativeInterop/cpp/src/command_server/command_server_file.cpp` | Updated `handleLoadFile()` to use `getFactory()`, added include for `render_context.hpp` |

### Multiplatform Considerations

The fix is designed to be multiplatform-friendly:

1. **Abstract base class pattern**: `getFactory()` is on the abstract `RenderContext` base class
2. **Virtual method**: Can be overridden for platform-specific behavior (e.g., custom image decoding wrapper)
3. **Common denominator**: `rive::gpu::RenderContext` works on all GPU platforms
4. **Fallback preserved**: `NoOpFactory` still available for tests/headless mode

For future platforms (Desktop, iOS, WASM), they only need to implement the `RenderContext` abstract methods:
- `initialize()` - Create the GPU context
- `destroy()` - Clean up resources
- `beginFrame()` - Bind the rendering surface
- `present()` - Swap buffers

The `getFactory()` implementation is inherited from the base class and works automatically once `riveContext` is initialized.

---

## Related Documentation

- [mprive_commandqueue_consolidated_plan.md](../aiplans/mprive_commandqueue_consolidated_plan.md) - Full architecture plan
- [t3_crash_when_running_demoapp.md](./t3_crash_when_running_demoapp.md) - Original crash logs

---

**Last Updated**: January 19, 2026 (SOLUTION IMPLEMENTED - Factory fix applied)