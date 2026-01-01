# mprive CommandQueue Architecture Analysis

**Date**: January 1, 2026  
**Question**: Should mprive adopt the CommandQueue architecture used in rive-android kotlin module?

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Understanding CommandQueue Architecture](#understanding-commandqueue-architecture)
3. [Why kotlin Module Uses CommandQueue](#why-kotlin-module-uses-commandqueue)
4. [Direct JNI vs CommandQueue Comparison](#direct-jni-vs-commandqueue-comparison)
5. [Potential Issues with Direct JNI](#potential-issues-with-direct-jni)
6. [Recommendation](#recommendation)
7. [Implementation Plan](#implementation-plan)

---

## Executive Summary

**TL;DR**: The kotlin module's CommandQueue architecture exists for **very good reasons** (thread safety, performance, Compose integration). However, for mprive's **phased approach**, we can start with direct JNI and **add CommandQueue later if needed**.

**Key Insight**: CommandQueue is **not just an abstraction** - it solves **real threading and performance problems** that we will likely encounter. We should plan for it but not implement it immediately.

**Recommendation**: 
1. ✅ Continue with direct JNI for Phase 2-3 (basic functionality)
2. ⚠️ Document threading requirements and limitations
3. ⏳ Evaluate performance in Phase 8
4. 🔄 Add CommandQueue architecture in Phase 9 if measurements show it's needed

---

## Understanding CommandQueue Architecture

### kotlin Module Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  UI Thread (Compose)                                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  RiveFile.fromSource()                               │   │
│  │    ↓                                                  │   │
│  │  CommandQueue.loadFile(bytes)                        │   │
│  │    ↓                                                  │   │
│  │  Queue command (async)                               │   │
│  │    ↓                                                  │   │
│  │  Return Future<FileHandle>                           │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ (command queued)
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  Render Thread (Dedicated Thread)                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  while (running) {                                   │   │
│  │      command = commandQueue.take()                   │   │
│  │      execute(command) // JNI call to native          │   │
│  │      complete(command.future)                        │   │
│  │  }                                                   │   │
│  └──────────────────────────────────────────────────────┘   │
│                         │                                    │
│                         ▼                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Native C++ (JNI)                                    │   │
│  │    ↓                                                  │   │
│  │  rive::File::import()                                │   │
│  │    ↓                                                  │   │
│  │  OpenGL operations (thread-local context)            │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Key Characteristics**:
- **Client/Server Model**: UI thread is client, render thread is server
- **Command Queue**: Producer-consumer pattern with thread-safe queue
- **Async Operations**: All operations return futures/suspending functions
- **Single Render Thread**: All Rive operations on same thread
- **OpenGL Context**: Tied to render thread, not UI thread

### mprive Current Architecture (Direct JNI)

```
┌─────────────────────────────────────────────────────────────┐
│  UI Thread (Compose)                                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  RiveFile.load(bytes)                                │   │
│  │    ↓                                                  │   │
│  │  nativeLoadFile(bytes) // Direct JNI call            │   │
│  │    ↓                                                  │   │
│  │  Blocks until complete                               │   │
│  │    ↓                                                  │   │
│  │  Return RiveFile                                     │   │
│  └──────────────────────────────────────────────────────┘   │
│                         │                                    │
│                         ▼                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Native C++ (JNI) - SAME THREAD                      │   │
│  │    ↓                                                  │   │
│  │  rive::File::import()                                │   │
│  │    ↓                                                  │   │
│  │  OpenGL operations (⚠️ on UI thread!)                │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Key Characteristics**:
- **Direct Calls**: Kotlin directly calls JNI on same thread
- **Synchronous**: Operations block until complete
- **No Thread Separation**: Native code runs on caller's thread
- **Simpler**: Fewer moving parts, easier to understand

---

## Why kotlin Module Uses CommandQueue

### 1. Thread Safety (OpenGL Context Management)

**Problem**: OpenGL contexts are **thread-local**. Once you create a context on a thread, all OpenGL operations for that context **must** happen on the same thread.

```cpp
// ❌ WRONG - This will crash or behave unpredictably
Thread A: eglMakeCurrent(display, surface, context)  // Create context
Thread B: glDrawArrays(...)  // Use context - CRASH!

// ✅ CORRECT - All operations on same thread
Thread A: eglMakeCurrent(display, surface, context)
Thread A: glDrawArrays(...)
Thread A: glFlush()
```

**kotlin Module Solution**: 
- Create OpenGL context on dedicated render thread
- All rendering operations queued to that thread
- Context never shared across threads

**mprive Current Approach**:
- No dedicated render thread
- If called from different threads, will have issues
- ⚠️ **Could crash on Desktop** if multiple composables render simultaneously

### 2. Performance (UI Thread Responsiveness)

**Problem**: Heavy Rive operations (file loading, complex rendering) can block UI thread and cause jank.

**Example Scenario**:
```kotlin
// Compose UI thread
@Composable
fun MyScreen() {
    val file = RiveFile.load(largeFileBytes)  // ⚠️ Blocks UI thread for 100ms!
    // UI freezes during loading
}
```

**kotlin Module Solution**:
```kotlin
@Composable
fun MyScreen() {
    val fileResult = rememberRiveFile(source)  // Async, doesn't block
    when (fileResult) {
        is Result.Loading -> CircularProgressIndicator()  // UI stays responsive
        is Result.Success -> RiveUI(fileResult.value)
    }
}
```

**mprive Current Approach**:
- All operations block caller thread
- If called on UI thread, UI will freeze
- ⚠️ **Will cause jank** for large files or complex animations

### 3. Compose Recomposition Handling

**Problem**: Compose can recompose frequently (on every state change, scroll, animation frame). Direct JNI calls during recomposition can be expensive.

**Example**:
```kotlin
@Composable
fun AnimatedRive(time: Long) {  // time changes every frame
    val artboard = file.artboard()  // ⚠️ Creates new artboard every frame!
    artboard.advance(time / 1000f)  // ⚠️ Heavy JNI call every frame!
    // Memory leak + performance issue
}
```

**kotlin Module Solution**:
- CommandQueue provides caching and reference counting
- Artboards are created once and reused
- State changes queue commands instead of recreating objects

**mprive Current Approach**:
- No built-in caching
- Relies on `remember {}` in Compose
- ⚠️ **Easy to misuse** and cause performance issues

### 4. Rive Runtime Thread Affinity

**Rive C++ Runtime Characteristics**:
- Many Rive objects are **not thread-safe**
- Animations, state machines have internal state
- Concurrent access can corrupt state
- Designed for single-threaded use

**kotlin Module Solution**:
- All Rive operations on single render thread
- No concurrent access possible
- Thread-safe by design

**mprive Current Approach**:
- Relies on Kotlin code not calling from multiple threads
- ⚠️ **No protection** against concurrent access
- ⚠️ **Could corrupt Rive state** if misused

### 5. Resource Lifecycle Management

**Problem**: Who owns Rive objects? When should they be deleted?

**kotlin Module Solution**:
```kotlin
// Reference counting
commandQueue.acquire("RiveFile")  // +1
val file = RiveFile(handle, commandQueue)
// ...
file.close()  // Decrements ref count
commandQueue.release("RiveFile", "closed")  // -1, deletes when 0
```

**mprive Current Approach**:
```kotlin
// Manual disposal
val file = RiveFile.load(bytes)
// ...
file.dispose()  // Must remember to call, or memory leak
```

**Issues with mprive**:
- ⚠️ Easy to forget `dispose()` → memory leak
- ⚠️ No protection against use-after-free
- ⚠️ No automatic cleanup when Composable leaves composition

---

## Direct JNI vs CommandQueue Comparison

| Aspect | Direct JNI (mprive) | CommandQueue (kotlin) | Winner |
|--------|---------------------|------------------------|---------|
| **Simplicity** | ⭐⭐⭐⭐⭐ Very simple | ⭐⭐ Complex | Direct JNI |
| **Lines of Code** | ~500 lines | ~2000+ lines | Direct JNI |
| **Thread Safety** | ⚠️ Manual (error-prone) | ✅ Built-in | CommandQueue |
| **Performance (large files)** | ❌ Blocks UI thread | ✅ Async, non-blocking | CommandQueue |
| **OpenGL Safety** | ⚠️ Context per thread | ✅ Single context | CommandQueue |
| **Compose Integration** | ⚠️ Manual `remember {}` | ✅ Automatic lifecycle | CommandQueue |
| **Memory Management** | ⚠️ Manual `dispose()` | ✅ Ref counting | CommandQueue |
| **Multiplatform** | ✅ Easy to share | ⚠️ Platform-specific threading | Direct JNI |
| **Debugging** | ✅ Straightforward | ⚠️ Async makes debugging harder | Direct JNI |
| **Production Readiness** | ⚠️ Risky for complex apps | ✅ Battle-tested | CommandQueue |

---

## Potential Issues with Direct JNI

### Issue 1: OpenGL Context Crashes (High Severity)

**Scenario**:
```kotlin
// Desktop app with multiple windows
Window1 {
    RiveUI(file1)  // Calls native from Compose thread A
}
Window2 {
    RiveUI(file2)  // Calls native from Compose thread B
}
// ❌ CRASH: Two threads trying to use OpenGL
```

**Probability**: High on Desktop, Low on Android (Android handles threading better)

**Impact**: App crashes, data loss

**Mitigation**: 
- Document "single thread only" requirement
- Or implement CommandQueue

### Issue 2: UI Jank (Medium Severity)

**Scenario**:
```kotlin
@Composable
fun GameScreen() {
    val bigFile = RiveFile.load(5MB_file)  // Takes 200ms
    // UI frozen for 200ms = noticeable stutter
}
```

**Probability**: High for large files, complex animations

**Impact**: Poor user experience, bad reviews

**Mitigation**:
- Use coroutines to load files off UI thread
- Or implement CommandQueue

### Issue 3: Memory Leaks (Medium Severity)

**Scenario**:
```kotlin
@Composable
fun AnimationScreen(url: String) {
    val file = RiveFile.load(fetchBytes(url))
    RiveUI(file)
    // ❌ file.dispose() never called!
    // Memory leak when navigation changes
}
```

**Probability**: Very high (easy to forget `dispose()`)

**Impact**: Memory leaks, eventual OOM crash

**Mitigation**:
- Implement AutoCloseable + use `use {}`
- Document best practices
- Or implement CommandQueue with automatic cleanup

### Issue 4: Race Conditions (Low-Medium Severity)

**Scenario**:
```kotlin
// Thread A
artboard.advance(0.016f)
artboard.draw(renderer)

// Thread B (simultaneously)
stateMachine.pointerDown(x, y)

// ❌ Rive internal state corrupted
```

**Probability**: Low if used correctly, Medium if misused

**Impact**: Subtle bugs, animation glitches, crashes

**Mitigation**:
- Document threading requirements
- Or implement CommandQueue

---

## Recommendation

### Short-Term (Phase 2-4): Continue with Direct JNI ✅

**Reasons**:
1. **Get basic functionality working first** - Prove the concept
2. **Simpler to implement** - Multiplatform code easier to share
3. **Adequate for simple use cases** - Single animation, careful threading
4. **Can measure later** - Need real performance data before optimizing

**Requirements**:
- ⚠️ **Document threading limitations** clearly
- ⚠️ Add `@MainThread` annotations where needed
- ⚠️ Implement `AutoCloseable` for memory safety
- ⚠️ Add warnings about OpenGL context issues

### Medium-Term (Phase 8-9): Evaluate and Decide ⏳

**Measurements to Take**:
1. **Performance**: Does file loading block UI? How long?
2. **Thread Safety**: Any crashes related to threading?
3. **Memory**: Any leaks from forgotten `dispose()` calls?
4. **User Feedback**: Reports of jank or freezing?

**Decision Criteria**:
- **IF** measurements show performance/threading issues → Implement CommandQueue
- **IF** simple use cases work fine → Keep direct JNI, add better documentation
- **IF** Desktop has more issues than Android → Consider platform-specific solutions

### Long-Term (Phase 9+): Add CommandQueue if Needed 🔄

**When to Implement**:
- Seeing actual crashes from OpenGL threading issues
- User complaints about UI freezing during file load
- Memory leak reports from production
- Need for advanced features (view models, complex state machines)

**Implementation Approach**:
1. Start with Android (proven architecture from kotlin module)
2. Adapt for Desktop (different threading model)
3. Keep direct JNI as fallback for simple cases
4. Provide both APIs: simple (direct JNI) and advanced (CommandQueue)

---

## Implementation Plan

### Phase 2-4: Direct JNI with Safeguards

```kotlin
// Step 1: Add threading annotations
@MainThread
actual fun load(bytes: ByteArray): RiveFile

// Step 2: Implement AutoCloseable
actual class RiveFile : AutoCloseable {
    actual override fun close() = dispose()
}

// Step 3: Add thread checking in JNI
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RiveFile_nativeLoadFile(...) {
    // Check we're on a safe thread
    if (!isRenderThreadOrMain()) {
        LOGE("RiveFile operations must be on main or render thread!");
        // Could throw exception or just warn
    }
    // ...
}

// Step 4: Document threading requirements
/**
 * **Threading**: All RiveFile operations must be called from the same thread.
 * Calling from multiple threads will result in undefined behavior and crashes.
 * 
 * **Best Practice**: Load files and create artboards in a coroutine, then use
 * the results on the UI thread.
 */
```

### Phase 8: Performance Evaluation

```kotlin
// Measure file loading time
val startTime = System.nanoTime()
val file = RiveFile.load(bytes)
val loadTime = (System.nanoTime() - startTime) / 1_000_000f
println("File loaded in ${loadTime}ms")

// If loadTime > 16ms → UI jank on 60fps
// If loadTime > 50ms → noticeable freeze

// Measure threading issues
// Monitor crash reports for OpenGL errors
// Check for race condition bugs in state machines
```

### Phase 9: CommandQueue Implementation (If Needed)

```kotlin
// Option 1: Port kotlin module's CommandQueue directly
class CommandQueue {
    private val renderThread = Thread { /* render loop */ }
    private val commandQueue = LinkedBlockingQueue<Command>()
    
    suspend fun <T> execute(command: Command<T>): T {
        return suspendCancellableCoroutine { cont ->
            commandQueue.offer(command.apply { 
                continuation = cont 
            })
        }
    }
}

// Option 2: Simpler coroutine-based approach
class RiveContext {
    private val scope = CoroutineScope(
        SupervisorJob() + 
        newSingleThreadContext("RiveRenderThread")
    )
    
    suspend fun <T> withRiveThread(block: () -> T): T {
        return withContext(scope.coroutineContext) {
            block()
        }
    }
}
```

---

## Conclusion

The kotlin module's CommandQueue architecture exists for **excellent reasons**:
1. ✅ Thread safety (OpenGL contexts)
2. ✅ Performance (non-blocking UI)
3. ✅ Reliability (proven in production)
4. ✅ Resource management (reference counting)

However, for mprive's **phased approach**:
1. ✅ Start with direct JNI (simpler, get basic functionality working)
2. ⚠️ Document limitations and threading requirements
3. 📊 Measure performance in real apps
4. 🔄 Add CommandQueue if measurements show it's needed

**We are not "missing" something** - we're making a **deliberate trade-off**:
- **Short-term**: Simplicity and speed of implementation
- **Long-term**: Plan to add CommandQueue if performance/threading issues arise

This is a **pragmatic approach** that lets us ship basic functionality quickly while keeping the door open for the more sophisticated architecture later.

---

**Recommended Next Steps**:
1. Continue implementing Phase 2-3 with direct JNI
2. Add threading documentation and `@MainThread` annotations
3. Implement `AutoCloseable` for better memory management
4. Plan Phase 8 performance measurements
5. Keep CommandQueue implementation as Phase 9 if needed

---

**End of CommandQueue Architecture Analysis**
