# Task: State Machine Skip Frames Optimization

**Date**: January 20, 2026
**Status**: PENDING (Future Work)
**Related**: t4_solve_greybox_rendering.md

---

## Background

During the investigation of the grey box rendering issue (t4), we discovered that mprive's `isSettled` optimization was causing animations to not play. This task documents the findings and provides recommendations for future battery-saving optimizations.

---

## The Problem with Current isSettled Implementation

### What mprive Was Doing (WRONG)

In `Rive.android.kt`:
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

When `advanceAndApply()` returned `false` (meaning `needsAdvance()` returns `false`), the `settledFlow` emitted an event, setting `isSettled = true`. This caused ALL subsequent frames to be skipped, breaking auto-playing animations.

### The `settled=1` Problem

From the logs:
```
AdvanceStateMachine command (smHandle=3, deltaTime=0.000000)
State machine advanced (handle=3, settled=1)  ← SETTLED IMMEDIATELY!

AdvanceStateMachine command (smHandle=3, deltaTime=0.008507)
State machine advanced (handle=3, settled=1)  ← STILL SETTLED!
```

The state machine reported `settled=1` on the very first advance and continued reporting settled on subsequent advances. This indicates the state machine thinks it doesn't need further updates, but animations should still be playing.

---

## How Original rive-android Handles This

### From `RiveFileController.advance()`

```kotlin
val stateMachinesToPause = mutableListOf<StateMachineInstance>()
stateMachines.forEach { stateMachineInstance ->
    if (playingStateMachines.contains(stateMachineInstance)) {
        val stillPlaying = resolveStateMachineAdvance(stateMachineInstance, elapsed)
        if (!stillPlaying) {
            stateMachinesToPause.add(stateMachineInstance)
        }
    }
}

// KEY: Only pause when elapsed > 0
if (elapsed > 0.0) {
    stateMachinesToPause.forEach { pause(stateMachine = it) }
}
```

### Original Behavior

1. **Advances ALL playing state machines EVERY FRAME** - regardless of `stillPlaying` return value
2. **Collects state machines to pause** when `stillPlaying=false`
3. **Only actually pauses them when `elapsed > 0`** - protects the first "settle" frame
4. **Paused state machines** are removed from `playingStateMachineSet` and won't advance anymore
5. **Drawing continues** as long as there are animations/state machines in the playing sets

### Key Insight: Paused vs Settled

- **Paused**: State machine removed from playing set, no more advances, but can be re-played
- **Settled**: The `needsAdvance()` return value indicates no pending state changes
- Original: Uses "paused" status to control the loop, not "settled"
- mprive: Was using "settled" to skip frames entirely (wrong!)

---

## Recommendations for Future Optimization

### Current Fix (Implemented in t4)

Removed the `isSettled` frame-skipping to match original behavior. Animations now play correctly.

### Future Battery Optimization Ideas

Once basic functionality is stable, consider these optimizations:

#### 1. Frame Rate Reduction When Settled

Instead of skipping frames entirely, reduce the frame rate when settled:

```kotlin
var targetFrameRate = if (isSettled) 10f else 60f // Hz
val frameInterval = (1000 / targetFrameRate).milliseconds

// Use frame rate limiting instead of skipping entirely
```

**Pros**: Saves battery while still responding to changes
**Cons**: Adds complexity, may have visible frame rate transitions

#### 2. Conditional Skip with Re-trigger

Only skip when settled AND no pending inputs:

```kotlin
if (isSettled && !hasPendingPointerEvents && !hasPendingViewModelChanges) {
    continue
}
```

**Pros**: More nuanced, better battery savings
**Cons**: Requires tracking all possible unsettling conditions

#### 3. Match Original "Pause When Truly Done" Logic

Implement the same pause logic as original:

```kotlin
var consecutiveSettledFrames = 0
val SETTLED_THRESHOLD = 3 // Number of frames to confirm truly settled

if (elapsed > 0 && !stillPlaying) {
    consecutiveSettledFrames++
    if (consecutiveSettledFrames >= SETTLED_THRESHOLD) {
        // Now it's safe to pause
        pause(stateMachine)
    }
} else {
    consecutiveSettledFrames = 0
}
```

**Pros**: More robust, matches original intent
**Cons**: Still advances for a few frames after settling

#### 4. Separate "Interactivity" From "Animation"

Distinguish between:
- **Interactive state machines**: Need to stay responsive even when settled
- **One-shot animations**: Can be paused when done

```kotlin
val isInteractive = stateMachine.hasPointerListeners || viewModelInstance != null

if (isSettled && !isInteractive && elapsed > 0) {
    pause(stateMachine)
}
```

**Pros**: Best of both worlds
**Cons**: Requires knowing state machine characteristics

---

## Why `settled=1` Happens Immediately

### Possible Causes

1. **State machine starts in settled state**: The initial "Entry" state may have no animations
2. **Animations require triggers**: Some .riv files need input to start animations
3. **Looping behavior**: Looping animations might report "settled" between loops
4. **State machine design**: The specific .riv file's state machine configuration

### Investigation Needed

- Test with different .riv files to see if `settled=1` behavior varies
- Check Rive documentation for `needsAdvance()` semantics
- Consider if `needsAdvance()` only refers to state transitions, not animation playback

---

## Files Involved

| File | Purpose |
|------|---------|
| `mprive/src/androidMain/kotlin/app/rive/mp/compose/Rive.android.kt` | Animation loop with isSettled check |
| `mprive/src/nativeInterop/cpp/src/command_server/command_server_statemachine.cpp` | Native advance handling |
| `kotlin/src/main/java/app/rive/runtime/kotlin/controllers/RiveFileController.kt` | Original implementation reference |

---

## Action Items

- [x] Fix immediate issue: Remove isSettled frame-skipping (done in t4)
- [ ] Add comprehensive testing with various .riv files
- [ ] Investigate why `settled=1` happens immediately for auto-playing animations
- [ ] Consider implementing one of the optimization strategies above
- [ ] Add configuration option for battery optimization level

---

**Last Updated**: January 20, 2026