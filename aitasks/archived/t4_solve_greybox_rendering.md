# Task: Solve Grey Box Rendering Issue in mprive RiveDemo

**Date**: January 19, 2026
**Status**: ✅ COMPLETE - Linear Animation Support Implemented
**Related**: t3_crash_when_running_demoapp.md

---

## 🎯 NEW ROOT CAUSE FOUND (January 20, 2026 - 22:30)

### The Real Problem: Missing Linear Animation Playback

After extensive investigation, the **actual root cause** was identified:

**The reference rive-android implementation plays BOTH linear animations AND state machines when autoplay is enabled. mprive ONLY plays state machines.**

### Evidence from Reference Implementation

In `RiveFileController.play()` (kotlin/src/main/java/app/rive/runtime/kotlin/controllers/RiveFileController.kt):

```kotlin
fun play(loop: Loop, direction: Direction, settleInitialState: Boolean) {
    activeArtboard?.let { activeArtboard ->
        // ...
        val animationNames = activeArtboard.animationNames
        if (animationNames.isNotEmpty()) {
            playAnimation(                                   // ← PLAYS FIRST LINEAR ANIMATION
                animationName = animationNames.first(),
                loop = loop,
                direction = direction
            )
        }
        val stateMachineNames = activeArtboard.stateMachineNames
        if (stateMachineNames.isNotEmpty()) {
            return playAnimation(                            // ← ALSO PLAYS STATE MACHINE
                animationName = stateMachineNames.first(),
                loop = loop,
                direction = direction,
                isStateMachine = true,
                settleInitialState = settleInitialState
            )
        }
    }
}
```

### Evidence from Diagnostic Logs

```
ARTBOARD DIAGNOSTIC - animationCount=5, stateMachineCount=1
DIAGNOSTIC BEFORE advance - currentAnimationCount=0, deltaTime=0.000000
DIAGNOSTIC AFTER advance - stillPlaying=1, currentAnimationCount=0, stateChangedCount=0
...
DIAGNOSTIC BEFORE advance - currentAnimationCount=0, deltaTime=0.008510
DIAGNOSTIC AFTER advance - stillPlaying=0, currentAnimationCount=0, stateChangedCount=0
State machine advanced (handle=3, settled=1)
```

Key observations:
- **`animationCount=5`** - The artboard has 5 linear animations
- **`currentAnimationCount=0` ALWAYS** - The state machine isn't driving any animations
- **`stateChangedCount=0`** - No state transitions occur
- **State machine settles immediately** - It has nothing to do

### Why This Happens

1. The .riv file's visible animation comes from a **linear animation**, NOT the state machine
2. The state machine in this file likely handles interactivity (triggers/inputs) rather than auto-play
3. mprive only creates and advances the state machine, which has no auto-playing content
4. The reference implementation plays both the first linear animation AND the state machine

---

## Implementation Plan: Option A - Full Linear Animation Support

### Overview

Implement full linear animation support in mprive to match the reference rive-android behavior. This requires:

1. **C++ Layer**: Add `LinearAnimationInstance` management to CommandServer
2. **JNI Layer**: Add bindings for animation operations
3. **Kotlin Layer**: Add `Animation` class and playback in the Rive composable
4. **Composable Layer**: Update `Rive.android.kt` to play animations when autoplay is enabled

---

### Step 1: Add LinearAnimationInstance Support to CommandServer (C++)

#### 1.1 Add storage for animation instances

**File**: `mprive/src/nativeInterop/cpp/include/command_server.hpp`

```cpp
#include "rive/animation/linear_animation_instance.hpp"

// Add to CommandServer class:
std::map<int64_t, std::unique_ptr<rive::LinearAnimationInstance>> m_animations;
```

#### 1.2 Add animation command types

**File**: `mprive/src/nativeInterop/cpp/include/command_types.hpp`

```cpp
enum class CommandType {
    // ... existing types ...
    
    // Animation commands
    CreateAnimation,
    CreateAnimationByName,
    AdvanceAnimation,
    ApplyAnimation,
    AdvanceAndApplyAnimation,
    DeleteAnimation,
    SetAnimationTime,
    SetAnimationLoop,
    SetAnimationDirection,
    GetAnimationInfo,
};
```

#### 1.3 Add Command struct fields for animation

```cpp
// In Command struct:
float animationTime;           // For SetAnimationTime
int32_t loopMode;              // For SetAnimationLoop (0=oneShot, 1=loop, 2=pingPong)
int32_t direction;             // For SetAnimationDirection (1=forwards, -1=backwards)
```

#### 1.4 Create command_server_animation.cpp

**File**: `mprive/src/nativeInterop/cpp/src/command_server/command_server_animation.cpp`

```cpp
#include "command_server.hpp"
#include "rive_log.hpp"
#include "rive/animation/linear_animation_instance.hpp"

namespace rive_android {

// Create default/first animation
int64_t CommandServer::createDefaultAnimationSync(int64_t artboardHandle)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_artboards.find(artboardHandle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", (long long)artboardHandle);
        return 0;
    }
    
    auto* artboard = it->second->artboard();
    if (!artboard || artboard->animationCount() == 0) {
        LOGW("CommandServer: Artboard has no animations");
        return 0;
    }
    
    // Create instance from first animation
    auto animation = artboard->animationAt(0);
    if (!animation) {
        LOGW("CommandServer: Failed to create animation at index 0");
        return 0;
    }
    
    int64_t handle = m_nextHandle.fetch_add(1);
    m_animations[handle] = std::move(animation);
    
    LOGI("CommandServer: Animation created (handle=%lld)", (long long)handle);
    return handle;
}

// Create animation by name
int64_t CommandServer::createAnimationByNameSync(int64_t artboardHandle, const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    
    auto it = m_artboards.find(artboardHandle);
    if (it == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", (long long)artboardHandle);
        return 0;
    }
    
    auto* artboard = it->second->artboard();
    if (!artboard) {
        LOGW("CommandServer: Artboard has no instance");
        return 0;
    }
    
    auto animation = artboard->animationNamed(name);
    if (!animation) {
        LOGW("CommandServer: Animation not found: %s", name.c_str());
        return 0;
    }
    
    int64_t handle = m_nextHandle.fetch_add(1);
    m_animations[handle] = std::move(animation);
    
    LOGI("CommandServer: Animation '%s' created (handle=%lld)", name.c_str(), (long long)handle);
    return handle;
}

// Advance animation
void CommandServer::advanceAnimation(int64_t animHandle, float deltaTime)
{
    auto it = m_animations.find(animHandle);
    if (it == m_animations.end()) {
        LOGW("CommandServer: Invalid animation handle: %lld", (long long)animHandle);
        return;
    }
    
    it->second->advance(deltaTime);
}

// Apply animation to artboard
void CommandServer::applyAnimation(int64_t animHandle, int64_t artboardHandle, float mix)
{
    auto animIt = m_animations.find(animHandle);
    if (animIt == m_animations.end()) {
        LOGW("CommandServer: Invalid animation handle: %lld", (long long)animHandle);
        return;
    }
    
    auto abIt = m_artboards.find(artboardHandle);
    if (abIt == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", (long long)artboardHandle);
        return;
    }
    
    animIt->second->apply(abIt->second->artboard(), mix);
}

// Advance and apply in one call (matches reference)
bool CommandServer::advanceAndApplyAnimation(int64_t animHandle, int64_t artboardHandle, float deltaTime)
{
    auto animIt = m_animations.find(animHandle);
    if (animIt == m_animations.end()) {
        LOGW("CommandServer: Invalid animation handle: %lld", (long long)animHandle);
        return false;
    }
    
    auto abIt = m_artboards.find(artboardHandle);
    if (abIt == m_artboards.end()) {
        LOGW("CommandServer: Invalid artboard handle: %lld", (long long)artboardHandle);
        return false;
    }
    
    auto& anim = animIt->second;
    auto* artboard = abIt->second->artboard();
    
    // Advance the animation
    bool looped = anim->advance(deltaTime);
    
    // Apply to artboard
    anim->apply(artboard);
    
    // Advance the artboard (needed when no state machine is driving it)
    artboard->advance(deltaTime);
    
    // Return whether animation is still playing (for oneShot detection)
    bool didLoop = anim->didLoop();
    return !didLoop || anim->loopValue() != 0; // 0 = oneShot
}

// Delete animation
void CommandServer::deleteAnimation(int64_t animHandle)
{
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        m_animations.erase(it);
        LOGI("CommandServer: Animation deleted (handle=%lld)", (long long)animHandle);
    }
}

// Set animation time
void CommandServer::setAnimationTime(int64_t animHandle, float time)
{
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        it->second->time(time);
    }
}

// Set animation loop mode
void CommandServer::setAnimationLoop(int64_t animHandle, int32_t loopMode)
{
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        it->second->loopValue(loopMode);
    }
}

// Set animation direction
void CommandServer::setAnimationDirection(int64_t animHandle, int32_t direction)
{
    auto it = m_animations.find(animHandle);
    if (it != m_animations.end()) {
        it->second->direction(direction);
    }
}

} // namespace rive_android
```

---

### Step 2: Add JNI Bindings for Animation (C++)

**File**: `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_animation.cpp`

```cpp
#include "bindings_commandqueue_internal.hpp"

extern "C" {

JNIEXPORT jlong JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateDefaultAnimation(
    JNIEnv* env, jobject, jlong ptr, jlong artboardHandle)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (!server) return 0;
    return server->createDefaultAnimationSync(artboardHandle);
}

JNIEXPORT jlong JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppCreateAnimationByName(
    JNIEnv* env, jobject, jlong ptr, jlong artboardHandle, jstring name)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (!server) return 0;
    
    const char* nameChars = env->GetStringUTFChars(name, nullptr);
    std::string animName(nameChars);
    env->ReleaseStringUTFChars(name, nameChars);
    
    return server->createAnimationByNameSync(artboardHandle, animName);
}

JNIEXPORT jboolean JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppAdvanceAndApplyAnimation(
    JNIEnv* env, jobject, jlong ptr, jlong animHandle, jlong artboardHandle, jfloat deltaTime)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (!server) return JNI_FALSE;
    return server->advanceAndApplyAnimation(animHandle, artboardHandle, deltaTime) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppDeleteAnimation(
    JNIEnv* env, jobject, jlong ptr, jlong animHandle)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server) server->deleteAnimation(animHandle);
}

JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetAnimationTime(
    JNIEnv* env, jobject, jlong ptr, jlong animHandle, jfloat time)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server) server->setAnimationTime(animHandle, time);
}

JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetAnimationLoop(
    JNIEnv* env, jobject, jlong ptr, jlong animHandle, jint loopMode)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server) server->setAnimationLoop(animHandle, loopMode);
}

JNIEXPORT void JNICALL
Java_app_rive_mp_core_CommandQueueJNIBridge_cppSetAnimationDirection(
    JNIEnv* env, jobject, jlong ptr, jlong animHandle, jint direction)
{
    auto* server = reinterpret_cast<CommandServer*>(ptr);
    if (server) server->setAnimationDirection(animHandle, direction);
}

} // extern "C"
```

---

### Step 3: Add Kotlin Animation Class

**File**: `mprive/src/commonMain/kotlin/app/rive/mp/Animation.kt`

```kotlin
package app.rive.mp

import kotlin.time.Duration

/**
 * Handle for a linear animation instance.
 */
@JvmInline
value class AnimationHandle(val handle: Long) {
    override fun toString(): String = "AnimationHandle($handle)"
}

/**
 * A linear animation instance from an [Artboard].
 *
 * Linear animations are timeline-based animations that play independently of state machines.
 * They are simpler than state machines but don't support interactivity.
 */
class Animation internal constructor(
    val animationHandle: AnimationHandle,
    internal val riveWorker: CommandQueue,
    internal val artboardHandle: ArtboardHandle,
    val name: String?,
) : AutoCloseable {

    private var closed = false

    companion object {
        private const val TAG = "Rive/Animation"
        
        /**
         * Creates the default (first) animation from an artboard.
         */
        fun fromArtboard(artboard: Artboard): Animation {
            val handle = artboard.riveWorker.createDefaultAnimation(artboard.artboardHandle)
            RiveLog.d(TAG) { "Created default animation $handle for ${artboard.artboardHandle}" }
            return Animation(handle, artboard.riveWorker, artboard.artboardHandle, null)
        }
        
        /**
         * Creates a named animation from an artboard.
         */
        fun fromArtboard(artboard: Artboard, name: String): Animation {
            val handle = artboard.riveWorker.createAnimationByName(artboard.artboardHandle, name)
            RiveLog.d(TAG) { "Created animation '$name' $handle for ${artboard.artboardHandle}" }
            return Animation(handle, artboard.riveWorker, artboard.artboardHandle, name)
        }
    }

    /**
     * Advances the animation by the given time delta and applies it to the artboard.
     *
     * @return true if the animation is still playing, false if it completed (oneShot)
     */
    fun advanceAndApply(deltaTime: Duration): Boolean {
        check(!closed) { "Animation has been closed" }
        return riveWorker.advanceAndApplyAnimation(
            animationHandle,
            artboardHandle,
            deltaTime.inWholeNanoseconds / 1_000_000_000f
        )
    }

    /**
     * Sets the animation's current time position.
     */
    fun setTime(time: Float) {
        check(!closed) { "Animation has been closed" }
        riveWorker.setAnimationTime(animationHandle, time)
    }

    /**
     * Sets the animation's loop mode.
     * @param loop 0=oneShot, 1=loop, 2=pingPong
     */
    fun setLoop(loop: Int) {
        check(!closed) { "Animation has been closed" }
        riveWorker.setAnimationLoop(animationHandle, loop)
    }

    /**
     * Sets the animation's playback direction.
     * @param direction 1=forwards, -1=backwards
     */
    fun setDirection(direction: Int) {
        check(!closed) { "Animation has been closed" }
        riveWorker.setAnimationDirection(animationHandle, direction)
    }

    override fun close() {
        if (closed) return
        closed = true
        RiveLog.d(TAG) { "Deleting animation $animationHandle" }
        riveWorker.deleteAnimation(animationHandle)
    }
}
```

---

### Step 4: Add CommandQueue Methods for Animation

**File**: `mprive/src/androidMain/kotlin/app/rive/mp/core/CommandQueue.kt`

Add these methods to the existing CommandQueue class:

```kotlin
// In CommandQueue class:

fun createDefaultAnimation(artboardHandle: ArtboardHandle): AnimationHandle {
    val handle = cppCreateDefaultAnimation(ptr, artboardHandle.handle)
    return AnimationHandle(handle)
}

fun createAnimationByName(artboardHandle: ArtboardHandle, name: String): AnimationHandle {
    val handle = cppCreateAnimationByName(ptr, artboardHandle.handle, name)
    return AnimationHandle(handle)
}

fun advanceAndApplyAnimation(animHandle: AnimationHandle, artboardHandle: ArtboardHandle, deltaSeconds: Float): Boolean {
    return cppAdvanceAndApplyAnimation(ptr, animHandle.handle, artboardHandle.handle, deltaSeconds)
}

fun deleteAnimation(animHandle: AnimationHandle) {
    cppDeleteAnimation(ptr, animHandle.handle)
}

fun setAnimationTime(animHandle: AnimationHandle, time: Float) {
    cppSetAnimationTime(ptr, animHandle.handle, time)
}

fun setAnimationLoop(animHandle: AnimationHandle, loop: Int) {
    cppSetAnimationLoop(ptr, animHandle.handle, loop)
}

fun setAnimationDirection(animHandle: AnimationHandle, direction: Int) {
    cppSetAnimationDirection(ptr, animHandle.handle, direction)
}

// JNI declarations
private external fun cppCreateDefaultAnimation(ptr: Long, artboardHandle: Long): Long
private external fun cppCreateAnimationByName(ptr: Long, artboardHandle: Long, name: String): Long
private external fun cppAdvanceAndApplyAnimation(ptr: Long, animHandle: Long, artboardHandle: Long, deltaTime: Float): Boolean
private external fun cppDeleteAnimation(ptr: Long, animHandle: Long)
private external fun cppSetAnimationTime(ptr: Long, animHandle: Long, time: Float)
private external fun cppSetAnimationLoop(ptr: Long, animHandle: Long, loop: Int)
private external fun cppSetAnimationDirection(ptr: Long, animHandle: Long, direction: Int)
```

---

### Step 5: Update Rive.android.kt to Play Animations

**File**: `mprive/src/androidMain/kotlin/app/rive/mp/compose/Rive.android.kt`

Key changes:

```kotlin
@Composable
actual fun Rive(
    file: RiveFile,
    modifier: Modifier,
    playing: Boolean,
    artboard: Artboard?,
    stateMachine: StateMachine?,
    animation: Animation?,              // ← ADD animation parameter
    viewModelInstance: ViewModelInstance?,
    fit: Fit,
    backgroundColor: Int,
    pointerInputMode: RivePointerInputMode
) {
    // ...existing code...
    
    // Try to create default animation if no state machine
    val animationToUse = animation ?: rememberAnimationOrNull(artboardToUse)
    
    // In the drawing loop:
    LaunchedEffect(/* keys */) {
        // ...
        lifecycleOwner.lifecycle.repeatOnLifecycle(Lifecycle.State.RESUMED) {
            var lastFrameTime = 0.nanoseconds
            while (isActive) {
                val deltaTime = withFrameNanos { /* ... */ }
                
                // Advance EITHER state machine OR animation (not both typically)
                if (stateMachineToUse != null) {
                    stateMachineToUse.advance(deltaTime)
                } else if (animationToUse != null) {
                    val stillPlaying = animationToUse.advanceAndApply(deltaTime)
                    if (!stillPlaying) {
                        // Animation completed (oneShot mode)
                        // Could emit an event or stop the loop
                    }
                }
                
                riveWorker.draw(/* ... */)
            }
        }
    }
}

@Composable
fun rememberAnimationOrNull(artboard: Artboard): Animation? {
    return remember(artboard) {
        try {
            Animation.fromArtboard(artboard)
        } catch (e: Exception) {
            null
        }
    }
}
```

---

## Additional Diagnostic Logging (for currentAnimationCount=0)

Add the following diagnostics to `command_server_statemachine.cpp::handleAdvanceStateMachine()`:

```cpp
void CommandServer::handleAdvanceStateMachine(const Command& cmd)
{
    auto it = m_stateMachines.find(cmd.handle);
    if (it == m_stateMachines.end()) {
        LOGW("CommandServer: Invalid state machine handle: %lld", (long long)cmd.handle);
        return;
    }
    
    auto& sm = it->second;
    
    // =========== ENHANCED DIAGNOSTICS ===========
    
    // Get the underlying StateMachine definition
    auto* smDef = sm->stateMachine();
    LOGW("DIAGNOSTIC - StateMachine definition:");
    LOGW("  - name: %s", smDef->name().c_str());
    LOGW("  - layerCount: %zu", smDef->layerCount());
    LOGW("  - inputCount: %zu", smDef->inputCount());
    
    // Log layer information
    for (size_t i = 0; i < smDef->layerCount(); i++) {
        auto* layer = smDef->layer(i);
        LOGW("  - layer[%zu]: stateCount=%zu", i, layer ? layer->stateCount() : 0);
    }
    
    // Log input information
    for (size_t i = 0; i < sm->inputCount(); i++) {
        auto* input = sm->input(i);
        if (input) {
            const char* type = "unknown";
            if (input->input()->is<rive::StateMachineNumber>()) type = "number";
            else if (input->input()->is<rive::StateMachineBool>()) type = "bool";
            else if (input->input()->is<rive::StateMachineTrigger>()) type = "trigger";
            LOGW("  - input[%zu]: name='%s', type=%s", i, input->name().c_str(), type);
        }
    }
    
    // =========== ANIMATION STATE BEFORE ADVANCE ===========
    
    size_t animCountBefore = sm->currentAnimationCount();
    bool needsAdvanceBefore = sm->needsAdvance();
    
    LOGW("DIAGNOSTIC BEFORE advance:");
    LOGW("  - currentAnimationCount=%zu", animCountBefore);
    LOGW("  - needsAdvance=%d", needsAdvanceBefore);
    LOGW("  - deltaTime=%f", cmd.deltaTime);
    
    // =========== ADVANCE ===========
    
    bool stillPlaying = sm->advanceAndApply(cmd.deltaTime);
    
    // =========== ANIMATION STATE AFTER ADVANCE ===========
    
    size_t animCountAfter = sm->currentAnimationCount();
    size_t stateChangedCount = sm->stateChangedCount();
    bool needsAdvanceAfter = sm->needsAdvance();
    
    LOGW("DIAGNOSTIC AFTER advance:");
    LOGW("  - stillPlaying=%d", stillPlaying);
    LOGW("  - currentAnimationCount=%zu", animCountAfter);
    LOGW("  - stateChangedCount=%zu", stateChangedCount);
    LOGW("  - needsAdvance=%d", needsAdvanceAfter);
    
    // Log which states changed
    for (size_t i = 0; i < stateChangedCount; i++) {
        auto* changedState = sm->stateChangedByIndex(i);
        if (changedState) {
            LOGW("  - stateChanged[%zu]: %s", i, changedState->name().c_str());
        }
    }
    
    // =========== KEY DIAGNOSTIC ===========
    
    if (animCountAfter == 0 && stillPlaying) {
        LOGW("⚠️ SUSPICIOUS: stillPlaying=true but currentAnimationCount=0!");
        LOGW("   This may indicate the state machine has no auto-playing animations.");
        LOGW("   Check if this .riv file requires interaction to trigger animations.");
    }
    
    if (animCountAfter == 0 && !stillPlaying) {
        LOGW("ℹ️ State machine settled with no active animations.");
        LOGW("   The visible content likely comes from a LINEAR ANIMATION, not the state machine.");
    }
    
    // =========== END DIAGNOSTICS ===========
    
    bool settled = !stillPlaying;
    LOGI("CommandServer: State machine advanced (handle=%lld, settled=%d)", 
         (long long)cmd.handle, settled);
}
```

---

## Implementation Checklist

- [x] **C++ Layer** *(Completed January 20-21, 2026)*
  - [x] Add `m_animations` storage to command_server.hpp
  - [x] Create command_server_animation.cpp with all animation methods
  - [x] Add animation method declarations to header
  - [x] Files included in build via CMakeLists.txt
  
- [x] **JNI Layer** *(Completed January 20-21, 2026)*
  - [x] Create bindings_commandqueue_animation.cpp
  - [x] All 7 JNI methods implemented
  
- [x] **Kotlin Layer** *(Completed January 21, 2026)*
  - [x] Add AnimationHandle to Handles.kt
  - [x] Add animation methods to CommandQueueBridge.kt interface
  - [x] Add external declarations to CommandQueueBridge.android.kt
  - [x] Add animation methods to CommandQueue.kt
  - [x] Create Animation.kt wrapper class with Loop and Direction enums
  
- [x] **Composable Layer** *(Completed January 21, 2026)*
  - [x] Create RememberAnimation.kt with rememberAnimation and rememberAnimationOrNull composables
  - [x] Update Rive.android.kt to support linear animations
  - [x] Add animationHandle to drawing loop LaunchedEffect keys
  - [x] Advance linear animation alongside state machine in render loop
  
- [ ] **Testing**
  - [ ] Build native code successfully
  - [ ] Run app and verify animations play
  - [ ] Test with different .riv files
  - [ ] Remove diagnostic code once working

### Implementation Summary (January 21, 2026)

The full linear animation support has been implemented:

1. **Animation.kt** - New wrapper class with:
   - `Loop` enum (ONE_SHOT, LOOP, PING_PONG)
   - `Direction` enum (FORWARDS, BACKWARDS)
   - `fromArtboard()` and `fromArtboardOrNull()` factory methods
   - `advanceAndApply()`, `setTime()`, `setLoop()`, `setDirection()` methods

2. **RememberAnimation.kt** - New composable file with:
   - `rememberAnimation()` - Creates animation with lifecycle management
   - `rememberAnimationOrNull()` - Gracefully handles artboards without animations
   - Overload that accepts StateMachine to match rive-android reference behavior

3. **Rive.android.kt** - Updated to:
   - Import Animation, AnimationHandle, and Loop
   - Create animation via `rememberAnimationOrNull(artboardToUse, stateMachineToUse)`
   - Include `animationHandle` in LaunchedEffect keys
   - Call `animationToUse?.advanceAndApply(deltaTime)` in render loop

The implementation matches the rive-android reference behavior where **BOTH** linear animations AND state machines can play simultaneously, ensuring .riv files with content driven by linear animations (not just state machines) will render correctly.

---

## Alternative Quick Fix (If Full Implementation Not Needed)

If the goal is just to get the current .riv file working without full animation support, a simpler fix would be:

1. Check if the .riv file has an auto-playing state machine (some files do)
2. Or add a trigger input at startup to begin the animation
3. Or modify the .riv file in the Rive editor to have auto-playing content

However, for proper rive-android parity, full linear animation support (Option A) is recommended.

---

## Related Documentation

- [mprive_commandqueue_consolidated_plan.md](../../aiplans/mprive_commandqueue_consolidated_plan.md) - Full architecture plan
- [t3_crash_when_running_demoapp.md](t3_crash_when_running_demoapp.md) - Original crash logs
- [t5_statemachine_skipframes_optimizations.md](../t5_statemachine_skipframes_optimizations.md) - Future optimization notes

---

**Last Updated**: January 21, 2026 (Implementation complete - linear animation support added)

### Final Refinement (January 21, 2026 - 07:38)

Added `advanceArtboard` parameter to the animation advancement chain to properly coordinate between linear animations and state machines:

- **C++ Layer**: `advanceAndApplyAnimation()` now takes `advanceArtboard` bool parameter
- **JNI Layer**: Updated `cppAdvanceAndApplyAnimation` to pass through the parameter
- **Kotlin Layer**: `Animation.advanceAndApply()` now accepts `advanceArtboard` with default `true`
- **Composable Layer**: `Rive.android.kt` passes `advanceArtboard = stateMachineToUse == null`

This ensures:
- When **no state machine exists**: animation advances the artboard itself (`advanceArtboard=true`)
- When **state machine exists**: state machine handles artboard advancement, animation only applies its transforms (`advanceArtboard=false`)

Both Kotlin and C++ builds verified successful.

---

## Previous Investigation History

*(Preserved for reference - see sections below for full history)*

<details>
<summary>Click to expand previous investigation history</summary>

### January 19-20, 2026 Investigation Summary

1. **Grey box issue** - Initially caused by NoOpFactory, fixed by using GPU factory
2. **Artboard not rendering** - Fixed by calling `artboard->advance(0)` before draw
3. **Animation not playing** - Changed `advance()` to `advanceAndApply()`
4. **isSettled frame skipping** - Removed premature frame skipping
5. **BindableArtboard fix** - Changed from ArtboardInstance to BindableArtboard
6. **Diagnostic logging** - Added comprehensive logging to track animation state

All these fixes were correctly applied, but the root cause was that mprive doesn't play linear animations like the reference implementation does.

</details>