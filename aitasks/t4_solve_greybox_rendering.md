# Task: Solve Grey Box Rendering Issue in mprive RiveDemo

**Date**: January 19, 2026
**Status**: PENDING
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

## CONTINUED INVESTIGATION (January 20, 2026)

### Factory Fix Did NOT Solve the Issue

The factory fix from January 19 was correctly applied - logs confirm:
```
CommandServer: Using GPU factory from RenderContext
```

However, the grey box persists. The issue is deeper than the factory selection.

---

## New Diagnostic Approach (January 20, 2026)

### GL Pipeline Diagnostic Test

Added a raw OpenGL test to verify the rendering pipeline independently of Rive:
- Draw a bright red test quad using direct GL calls
- Test both BEFORE and AFTER Rive's `flush()` call

### Key Finding #1: Raw GL Works AFTER Flush

**Test**: Draw red quad AFTER `riveContext->flush()` but BEFORE `present()`

**Result**: ✅ RED QUAD APPEARS ON SCREEN

This confirms:
- GL pipeline is fully functional
- EGL surface is correctly bound
- `eglSwapBuffers` presents to the correct surface
- FBO 0 correctly points to the window framebuffer

### Key Finding #2: Artboard Has Plenty of Content

Artboard diagnostic logging shows:
```
ARTBOARD DIAGNOSTIC - name='New Artboard', size=1000x1000
ARTBOARD DIAGNOSTIC - objectCount=1147, animationCount=5, stateMachineCount=1
ARTBOARD DIAGNOSTIC - object[0]: coreType=1   (Artboard)
ARTBOARD DIAGNOSTIC - object[1]: coreType=20  (Transform)
ARTBOARD DIAGNOSTIC - object[2]: coreType=22  (Component)
ARTBOARD DIAGNOSTIC - object[3]: coreType=19  (Component)
ARTBOARD DIAGNOSTIC - object[5]: coreType=2   (Node)
ARTBOARD DIAGNOSTIC - object[6]: coreType=3   (Shape)
ARTBOARD DIAGNOSTIC - object[7]: coreType=16  (SolidColor)
ARTBOARD DIAGNOSTIC - object[8]: coreType=5   (Fill/Stroke)
... and 1137 more objects
```

The artboard contains:
- 1147 objects (shapes, fills, colors, transforms)
- 5 animations
- 1 state machine

This is NOT an empty artboard - it has substantial visual content.

### Key Finding #3: Rive's flush() Clears But Doesn't Render Content

The rendering sequence:
1. `riveContext->beginFrame()` - queues clear operation with grey color ✓
2. `artboard->draw(&renderer)` - should queue draw commands for 1147 objects
3. `riveContext->flush({.renderTarget})` - executes queued commands

**Observed behavior**:
- Clear operation executes (we see grey background) ✓
- Artboard content does NOT appear ✗
- No GL errors reported ✓
- FBO binding is 0 after flush ✓

**Conclusion**: The draw commands either:
1. Are not being generated by `artboard->draw()`
2. Are being generated but filtered out during flush
3. Are being rendered to a different target than expected

---

## Current Hypothesis

### Render Target Configuration Issue

The render target is created with:
```
CreateRiveRenderTarget: created renderTarget=... (FBO=0, 984x1332, samples=0)
```

Key observations:
- `FBO=0` - Uses default framebuffer
- `samples=0` - No MSAA
- Created when **PBuffer** surface is current (1x1 viewport)
- Used when **window** surface is current (984x1332)

**Potential issue**: `FramebufferRenderTargetGL::make()` might capture some GL state at creation time that's incompatible with the window surface context.

### Alternative Hypothesis: RiveRenderer State

The `rive::RiveRenderer` might require specific initialization or state that we're not providing. The original rive-android uses a lambda pattern that might configure the renderer differently.

---

## Files Modified for Diagnostics (January 20, 2026)

| File | Changes |
|------|---------|
| `command_server_render.cpp` | Added red test quad after flush, artboard content logging |

### Diagnostic Code Location

The diagnostic code is in `handleDraw()` in `command_server_render.cpp`:
1. Artboard content logging (shows object count and types)
2. Red test quad drawing (after flush, before present)

**Note**: This diagnostic code should be removed once the issue is fixed.

---

## Next Steps for New Session

### Step 1: Compare Render Target Creation

Compare how render targets are created in:
1. **Original rive-android** (`kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp`)
2. **mprive** (`mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_rendering.cpp`)

Look for:
- Different parameters to `FramebufferRenderTargetGL::make()`
- Different GL context state expectations
- Different surface binding sequences

### Step 2: Add Draw Command Diagnostics

Verify if draw commands are actually being generated:
- Add logging inside `RiveRenderer` draw calls (if accessible)
- Count commands queued before/after `artboard->draw()`
- Check if Rive's command buffer is empty after draw

### Step 3: Test with Different Render Target Configuration

Try creating the render target:
- With MSAA (samples > 0)
- After window surface is bound (not during PBuffer)
- Using explicit FBO creation instead of FBO=0

### Step 4: Compare with Original Module

Run the original `:app` module with `:kotlin` to verify:
- Same .riv file renders correctly
- Capture the render target creation sequence
- Compare GL state during rendering

---

## Summary of Investigation Status

| Aspect | Status | Finding |
|--------|--------|---------|
| GL Pipeline | ✅ Working | Red quad renders after flush |
| EGL Surface | ✅ Working | eglMakeCurrent and eglSwapBuffers succeed |
| File Loading | ✅ Working | GPU factory used, 1147 objects loaded |
| Artboard Content | ✅ Present | Shapes, fills, colors, animations all present |
| Clear Operation | ✅ Working | Grey background appears |
| Artboard Rendering | ❌ NOT WORKING | Content not visible after flush |
| Root Cause | 🔍 Unknown | Likely render target or RiveRenderer config |

---

## Related Documentation

- [mprive_commandqueue_consolidated_plan.md](../aiplans/mprive_commandqueue_consolidated_plan.md) - Full architecture plan
- [t3_crash_when_running_demoapp.md](./t3_crash_when_running_demoapp.md) - Original crash logs
- [t4_logoutput.md](../aidata/t4_logoutput.md) - Latest diagnostic log output

---

**Last Updated**: January 20, 2026 (GL diagnostic confirms pipeline works, artboard content not rendering)

---

## CONTINUED INVESTIGATION (January 20, 2026 - Session 2)

### Fixes Applied This Session

1. **Sample Count Fix** (`bindings_commandqueue_rendering.cpp`)
   - Problem: Render target was created with `samples=0` because PBuffer has no MSAA
   - Fix: Ensure sample count is at least 1 when queried as 0
   ```cpp
   GLint actualSampleCount = (queriedSampleCount > 0) ? queriedSampleCount : 1;
   ```
   - Result: Logs confirm `queried samples=0, using samples=1`

2. **Blue Rectangle Diagnostic** (command_server_render.cpp)
   - Added RiveRenderer-based diagnostic before artboard draw
   - **CRASHED** when calling `renderContext->riveContext->makeRenderPaint()`
   - Temporarily disabled to continue testing

### Key Findings This Session

1. **Sample count fix applied** - Render target now uses samples=1 instead of 0
2. **Blue rectangle diagnostic crashes** - Suggests `riveContext->makeRenderPaint()` fails during frame
3. **Red quad still appears** - Raw GL after flush works
4. **Artboard content still not rendering** - Same grey box issue persists

### Why Blue Rectangle Crashed

The crash when calling `renderContext->riveContext->makeRenderPaint()` is significant:
- `rive::gpu::RenderContext` inherits from `rive::Factory` 
- Creating new render objects during a frame might not be supported
- The original implementation doesn't create new objects during draw - it uses objects created during file import
- This explains why artboard.draw() doesn't crash but blue rectangle does

### Next Investigation Direction

The artboard has 1147 objects (shapes, fills, etc.) but nothing renders. The objects were created during import using the GPU factory. When artboard->draw() is called, it should queue draw commands that flush() executes.

**Hypothesis**: The draw commands ARE being queued but something in the render target or viewport configuration prevents them from appearing.

**Next Steps**:
1. Add diagnostic to log the renderer's internal state (transforms, clip regions)
2. Check if Rive's flush() respects the current viewport or uses its own
3. Compare the exact parameters passed to flush() vs original implementation
4. Try setting viewport explicitly before flush (like the red quad does)

### Potential Fix: Viewport Before Flush

The red quad works because it explicitly sets viewport before drawing:
```cpp
glViewport(0, 0, cmd.surfaceWidth, cmd.surfaceHeight);
```

Maybe Rive's flush() expects a specific viewport configuration that isn't being set. Try adding `glViewport()` before `flush()`.

---

## Investigation Session 2 Summary (January 20, 2026 - 13:30)

### Extensive Comparison - No Differences Found

| Aspect | Original | mprive | Match? |
|--------|----------|--------|--------|
| RenderContext creation | `RenderContextGLImpl::MakeContext()` | `RenderContextGLImpl::MakeContext()` | ✅ |
| Render target | `new FramebufferRenderTargetGL(w, h, 0, samples)` | Same | ✅ |
| Flush call | `{.renderTarget = renderTarget}` | Same | ✅ |
| beginFrame params | `{width, height, clear, color}` | Same | ✅ |
| Factory for loading | `riveContext.get()` | `renderContext->getFactory()` → `riveContext.get()` | ✅ |

### All Applied Fixes
1. Sample count >= 1 (was 0)
2. Viewport set before flush
3. GPU factory used for file loading

### Remaining Mystery

The implementations are **functionally identical** yet:
- Original: Renders content correctly
- mprive: Grey background only (clear works, content doesn't)

### Possible Root Causes (Not Yet Tested)

1. **Artboard visibility state** - Artboard might be hidden/zero opacity
2. **Rive internal draw list empty** - draw() might not generate commands
3. **State machine interference** - SM might be setting visibility states
4. **Object initialization** - GPU objects might need warmup/first-frame behavior

### Next Steps for Future Session

1. **Check artboard visibility**:
   - Add diagnostic: `artboard->opacity()`, any visibility flags
   - Try `artboard->advance(0)` before draw to ensure state is updated

2. **Try without state machine**:
   - Draw artboard without creating/advancing state machine
   - This isolates whether SM is affecting visibility

3. **Compare with simpler .riv file**:
   - Test with a minimal .riv (single colored rectangle)
   - Isolates whether the file complexity is the issue

4. **Add Rive debug logging**:
   - Check if Rive has debug modes or logging
   - Track what happens inside artboard->draw()

---

## 🎉 BREAKTHROUGH! Rendering Now Works (January 20, 2026 - 13:35)

### The Fix

Adding `artboard->advance(0)` before `artboard->draw()` in `handleDraw()` **fixed the grey box issue**!

```cpp
// In handleDraw(), before artboard->draw():
artboard->advance(0);  // <-- THIS FIXED IT!
artboard->draw(&renderer);
```

### Why This Works

The artboard needs to have its state "advanced" at least once before drawing, even if the delta time is 0. This initializes the visual state of all components in the artboard hierarchy. Without this call, the artboard's render state was never computed, resulting in nothing being drawn.

### Current Status
- ✅ **Static content renders correctly**
- ❌ **Animation not playing** - Content appears but doesn't animate

### Root Cause Analysis

The issue was that artboard render state wasn't initialized:
1. File loads successfully with GPU factory ✅
2. Artboard and state machine created ✅
3. Draw is called, but artboard internal state was never "computed"
4. `artboard->advance(0)` initializes the render state
5. Now `artboard->draw()` has valid state to render

---

## Animation Not Working - Investigation Plan

### Current Behavior
- Static content renders ✅
- Animation doesn't play ❌
- State machine advances are being logged correctly

### Hypothesis: `advance(0)` is Overwriting State Machine Changes

The current flow:
1. Kotlin calls `advanceStateMachine(deltaTime)` → `sm->advance(dt)` 
2. Kotlin calls `draw()` → `artboard->advance(0)` + `artboard->draw()`

**Problem**: Calling `artboard->advance(0)` AFTER the state machine advance might be resetting/overwriting the animation state that the SM just computed.

### Proposed Investigation Steps

1. **Check if SM advance already advances artboard**
   - `StateMachineInstance::advance(dt)` might internally call artboard->advance()
   - If so, our extra `advance(0)` is redundant AND possibly harmful

2. **Try removing `artboard->advance(0)`**
   - If rendering breaks, we definitely need it
   - If rendering works but animation still broken, issue is elsewhere

3. **Try calling `artboard->advance(deltaTime)` instead**
   - Pass actual time delta instead of 0
   - Would properly advance both artboard state AND animations

4. **Check original implementation**
   - How does original rive-android handle artboard advance?
   - Is advance called in draw or separately?

5. **Verify timing between SM advance and draw**
   - Are SM changes being lost between commands?
   - Is there a synchronization issue?

### Key Questions to Answer

| Question | To Investigate |
|----------|----------------|
| Does SM::advance() call artboard::advance()? | Check Rive runtime source |
| Is artboard::advance(0) resetting animation state? | Try removing it |
| Should advance be called in draw or separately? | Compare with original |
| Is deltaTime being passed correctly? | Add logging |

### Files to Check

- `submodules/rive-runtime/include/rive/animation/state_machine_instance.hpp`
- `kotlin/src/main/cpp/src/bindings/bindings_command_queue.cpp` (original draw)
- `mprive/src/nativeInterop/cpp/src/command_server/command_server_statemachine.cpp`

### Next Session Tasks

1. **Check Rive documentation** on artboard vs state machine advance
2. **Examine original draw implementation** for advance patterns  
3. **Try removing advance(0)** to see what breaks
4. **Try passing actual deltaTime** to artboard advance
5. **Add deltaTime logging** to trace timing flow

---

## 🎉 ANIMATION FIX IMPLEMENTED (January 20, 2026 - 15:24)

### Root Cause Found

The problem was a **mismatch between state machine and artboard advance calls**:

1. **`StateMachineInstance::advance(seconds)`** - Only advances the state machine layers, does NOT advance the artboard
2. **`StateMachineInstance::advanceAndApply(seconds)`** - Advances BOTH the state machine AND calls `m_artboardInstance->advanceInternal(seconds, ...)` to advance the artboard

**mprive's previous flow (broken):**
1. `handleAdvanceStateMachine()` calls `sm->advance(deltaTime)` - only SM, NOT artboard
2. `handleDraw()` calls `artboard->advance(0)` - advances artboard with **0 delta**, overwriting animation!

### The Fix

**File 1: `command_server_statemachine.cpp` - `handleAdvanceStateMachine()`**
```cpp
// CHANGED FROM:
it->second->advance(cmd.deltaTime);

// TO:
it->second->advanceAndApply(cmd.deltaTime);
```

**File 2: `command_server_render.cpp` - `handleDraw()`**
```cpp
// REMOVED:
artboard->advance(0);

// REPLACED WITH comment explaining why it's not needed
```

### Why This Works

1. **First frame**: Kotlin calls `advanceStateMachine(0)` → `advanceAndApply(0)` initializes render state
2. **Subsequent frames**: Kotlin calls `advanceStateMachine(deltaTime)` → `advanceAndApply(deltaTime)` properly animates both SM and artboard
3. **Draw**: Just renders the current state without overwriting anything

### Files Modified

| File | Changes |
|------|---------|
| `command_server_statemachine.cpp` | Changed `advance()` to `advanceAndApply()` |
| `command_server_render.cpp` | Removed `artboard->advance(0)` call |

### Expected Behavior After Fix

- ✅ Static content renders correctly (from first frame's `advanceAndApply(0)`)
- ✅ Animations play correctly (state machine and artboard advance together)
- ✅ No more grey box on first render
- ✅ No animation state being overwritten

---

## CONTINUED INVESTIGATION - isSettled Optimization Issue (January 20, 2026 - 15:55)

### The advanceAndApply() Fix Was Correct But Incomplete

The change from `advance()` to `advanceAndApply()` was correct - it matches the original JNI binding. However, the animation still wasn't playing due to a separate issue in the Kotlin composable.

### Root Cause: `isSettled` Frame-Skipping

In `Rive.android.kt`, the animation loop was skipping ALL frames when `isSettled=true`:

```kotlin
while (isActive) {
    val deltaTime = withFrameNanos { ... }

    // Skip advance and draw when settled
    if (isSettled) {
        continue  // ← SKIPS ALL FRAMES!
    }

    stateMachineToUse?.advance(deltaTime)
    riveWorker.draw(...)
}
```

When `advanceAndApply()` returned `false` (`needsAdvance()=false`), the `settledFlow` emitted an event, setting `isSettled=true`. This caused ALL subsequent frames to be skipped.

### Log Evidence

```
State machine advanced (handle=3, settled=1)  ← SETTLED IMMEDIATELY on first frame!
```

The state machine reported `settled=1` on the very first advance (with deltaTime=0) and continued reporting settled, causing the loop to skip all frames.

### How Original rive-android Handles This

From `RiveFileController.advance()`:

```kotlin
// Only remove the state machines if the elapsed time was
// greater than 0. 0 elapsed time causes no changes so it's a no-op advance.
if (elapsed > 0.0) {
    stateMachinesToPause.forEach { pause(stateMachine = it) }
}
```

The original:
1. **Always advances state machines** - regardless of `stillPlaying` return value
2. **Only pauses when elapsed > 0** - protects the first settle frame
3. **Never skips frames** - paused state machines just aren't advanced

### The Fix

Removed the `isSettled` frame-skipping in `Rive.android.kt` to match original behavior:

```kotlin
// REMOVED:
if (isSettled) {
    continue
}

// Now always advances and draws while playing=true
stateMachineToUse?.advance(deltaTime)
riveWorker.draw(...)
```

### Files Modified

| File | Changes |
|------|---------|
| `mprive/src/androidMain/kotlin/app/rive/mp/compose/Rive.android.kt` | Removed `isSettled` frame-skipping |

### Future Optimization

The `isSettled` optimization is a battery-saving feature that should be revisited with more nuanced logic. See `aitasks/t5_statemachine_skipframes_optimizations.md` for recommendations.

---

**Last Updated**: January 20, 2026 (Added diagnostic logging for animation state)

---

## CONTINUED INVESTIGATION - Diagnostic Logging Added (January 20, 2026 - 18:45)

### Previous Fix Did Not Work

The `isSettled` frame-skipping fix and the `advanceAndApply()` change did not resolve the animation issue. The animation loop runs but animations don't play.

### Root Cause Identified: Wrong "Settled" Check

In `handleAdvanceStateMachine()`, the code was using `needsAdvance()` to determine if the state machine was "settled", but this is WRONG:

```cpp
// WRONG - needsAdvance() checks for pending INPUT changes, not animation state
bool settled = !it->second->needsAdvance();

// CORRECT - use the RETURN VALUE of advanceAndApply() which indicates if animations continue
bool stillPlaying = sm->advanceAndApply(cmd.deltaTime);
bool settled = !stillPlaying;
```

### Key Difference: needsAdvance() vs advanceAndApply() Return Value

| Method | What It Checks |
|--------|----------------|
| `needsAdvance()` | Returns `m_needsAdvance` flag - set when inputs change |
| `advanceAndApply()` return | Returns true if animations will CONTINUE playing |

The reference implementation returns `advanceAndApply()` result directly to Kotlin, which uses it to determine if the animation should continue.

### Diagnostic Logging Added

Added comprehensive diagnostics to `handleAdvanceStateMachine()` in `command_server_statemachine.cpp`:

```cpp
// BEFORE advance
size_t animCountBefore = sm->currentAnimationCount();
LOGW("DIAGNOSTIC BEFORE advance - currentAnimationCount=%zu, deltaTime=%f", ...);

// Capture return value!
bool stillPlaying = sm->advanceAndApply(cmd.deltaTime);

// AFTER advance
size_t animCountAfter = sm->currentAnimationCount();
size_t stateChangedCount = sm->stateChangedCount();
bool needsAdvanceFlag = sm->needsAdvance();

LOGW("DIAGNOSTIC AFTER advance - stillPlaying=%d, currentAnimationCount=%zu, stateChangedCount=%zu, needsAdvance=%d", ...);

// FIXED: Use return value instead of needsAdvance()
bool settled = !stillPlaying;
```

### What to Look For in New Logs

| Log Value | What It Means |
|-----------|---------------|
| `currentAnimationCount=0` | No animations are active |
| `stillPlaying=0` | Animations have completed or not started |
| `stateChangedCount>0` | State transitions occurred |
| `needsAdvance=0` but `stillPlaying=1` | Animation running, no pending inputs |

### Files Modified

| File | Changes |
|------|---------|
| `command_server_statemachine.cpp` | Added diagnostic logging, fixed settled check to use `stillPlaying` |

### Next Steps

1. **Run the app** and capture new logs
2. **Analyze** `currentAnimationCount` and `stillPlaying` values
3. **If `currentAnimationCount=0`**: The state machine may need a trigger to start
4. **If `stillPlaying=0`**: Animations may complete instantly or not be configured

---

## Related Documentation

- [mprive_commandqueue_consolidated_plan.md](../aiplans/mprive_commandqueue_consolidated_plan.md) - Full architecture plan
- [t3_crash_when_running_demoapp.md](./t3_crash_when_running_demoapp.md) - Original crash logs
- [t5_statemachine_skipframes_optimizations.md](./t5_statemachine_skipframes_optimizations.md) - Future optimization notes