# mprive Multiplatform Testing Strategy

**Project**: rive-android-fork-dec25  
**Module**: mprive (Kotlin Multiplatform)  
**Based on**: kotlin module Android tests (`kotlin/src/androidTest`)  
**Date**: January 1, 2026  
**Updated**: January 4, 2026  
**Note**: mprive uses `androidInstrumentedTest` directory (KMP Source Set Layout V2)
**Status**: Phase A & B Partially Implemented

---

## 📊 Implementation Status (January 4, 2026)

### Completed ✅

**Phase A: CommandQueue Foundation (75% Complete)**
- ✅ `MpCommandQueueLifecycleTest.kt` - 8 test methods (reference counting, acquire/release, disposal)
- ✅ `MpCommandQueueThreadSafetyTest.kt` - 3 test methods (concurrent operations)
- ✅ `MpCommandQueueHandleTest.kt` - 8 test methods (handle uniqueness, incrementing, validation)
- ⏸️ `MpCommandQueueTest.kt` - **DEFERRED** (requires Compose integration: `rememberCommandQueue()`, `MpComposeTestUtils`)

**Phase B: File & Artboard Operations (100% Complete)**
- ✅ `MpRiveFileLoadTest.kt` - 6 test methods (file loading, error handling, handle uniqueness)
- ✅ `MpRiveArtboardLoadTest.kt` - 10 test methods (artboard queries, creation, edge cases)

**Test Infrastructure (100% Complete)**
- ✅ `MpTestContext.kt` - Platform-agnostic test context (common + Android + Desktop + iOS)
- ✅ `MpTestResources.kt` - Resource loading abstraction (common + Android + Desktop)
- ✅ `MpCommandQueueTestUtil.kt` - Automatic message polling utility

**Test Resources (Phase B Complete)**
- ✅ 11 .riv files copied to `commonTest/resources/rive/`
- ✅ 1 .png asset file (eve.png)

### In Progress / Pending 🔄

**Phase A Blocker**
- ⏸️ **Compose Integration Not Implemented**
  - Missing: `rememberCommandQueue()` composable function
  - Missing: `MpComposeTestUtils.kt` and platform implementations
  - Impact: Cannot test Compose-based CommandQueue lifecycle
  - Recommendation: Defer until Compose integration is ready

**Phases C-G: Not Started ❌**
- Phase C: State Machines & Rendering (0 of 5 tests)
- Phase D: View Models & Properties (0 of 1 test - the massive 2,044-line test!)
- Phase E: Advanced Features (0 of 4 tests)
- Phase F: Platform-Specific Tests (0 of 7 tests)
- Phase G: Integration & Optimization (0 of 5 tests)

### Summary Statistics

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| **Tests Implemented** | 28 | 5 | 18% ✅ |
| **Phase A** | 4 tests | 3 tests | 75% ⏸️ (1 deferred) |
| **Phase B** | 2 tests | 2 tests | 100% ✅ |
| **Phases C-G** | 22 tests | 0 tests | 0% ❌ |
| **Test Resources** | 37 files | 12 files | 32% ✅ |

### Test Execution Commands

**Run All Tests:**
```bash
# Android (requires connected device/emulator)
./gradlew :mprive:connectedAndroidTest

# Desktop
./gradlew :mprive:desktopTest

# Both platforms
./gradlew :mprive:connectedAndroidTest :mprive:desktopTest
```

**Run Specific Test Suites:**
```bash
# CommandQueue tests only
./gradlew :mprive:desktopTest --tests "*CommandQueue*"

# File/Artboard tests only
./gradlew :mprive:desktopTest --tests "*File*" --tests "*Artboard*"
```

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Test Analysis](#test-analysis)
3. [Multiplatform Test Structure](#multiplatform-test-structure)
4. [Resource Management](#resource-management)
5. [Test Utilities](#test-utilities)
6. [Test Porting Plan by Phase](#test-porting-plan-by-phase)
7. [Test Execution](#test-execution)
8. [Coverage Targets](#coverage-targets)

---

## Executive Summary

### Source
The existing kotlin module has **31 test files** with **37 test resources** covering all aspects of the Rive runtime. These tests are comprehensive and production-proven.

### Strategy
We will port **~35% of tests** (the most critical ones) to multiplatform common tests, add **6 new CommandQueue-specific tests**, and create **4 platform-specific test suites**.

### Key Decisions
1. ✅ **Copy resources as needed** per phase (not all upfront)
2. ✅ **Rename with "Mp" prefix** (e.g., `MpRiveFileLoadTest`) for clarity
3. ✅ **Skip UI component tests** (Android View-specific, not needed for Compose)
4. ✅ **Lower priority**: Snapshot tests, Font tests (Phase 8+)

---

## Test Analysis

### Test Classification (31 Total Test Files)

#### **Category 1: Core File & Artboard** (3 tests) - ⭐⭐⭐ HIGH PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveFileLoadTest.kt` | ~80 | File loading, error handling, asset loaders | ✅ Phase B |
| `RiveArtboardLoadTest.kt` | ~150 | Artboard queries, access by name/index | ✅ Phase B |
| `RiveArtboardRendererTest.kt` | ~100 | Artboard rendering pipeline | ✅ Phase C |

**Total**: ~330 lines → Port to `commonTest`

---

#### **Category 2: Animation Operations** (3 tests) - ⭐⭐⭐ HIGH PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveAnimationLoadTest.kt` | ~120 | Animation queries, loading | ✅ Phase B |
| `RiveAnimationConfigurationsTest.kt` | ~200 | Animation mix values, configurations | ✅ Phase E |
| `RiveLinearAnimationInstanceAdvanceTest.kt` | ~150 | Animation timing, advancement | ✅ Phase C |

**Total**: ~470 lines → Port to `commonTest`

---

#### **Category 3: State Machine Operations** (5 tests) - ⭐⭐⭐ HIGH PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveStateMachineLoadTest.kt` | ~100 | State machine queries, loading | ✅ Phase C |
| `RiveStateMachineInstanceTest.kt` | ~250 | State machine advancement, inputs | ✅ Phase C |
| `RiveStateMachineConfigurationsTest.kt` | ~180 | State machine configurations | ✅ Phase C |
| `RiveStateMachineStateResolutionTest.kt` | ~200 | State transitions, resolution | ✅ Phase C |
| `RiveStateMachineTouchEventTest.kt` | ~150 | Touch event handling | ❌ Platform-specific (Phase F) |

**Total**: ~730 lines (580 to commonTest, 150 to platformTest)

---

#### **Category 4: View Models & Data Binding** (1 test) - ⭐⭐⭐ CRITICAL

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveDataBindingTest.kt` | **2,044** | **Everything**: VM/VMI creation, properties (number, string, boolean, enum, color, trigger, image, artboard), nested properties, list properties, property flows, bindable artboards, transfer, concurrent access | ✅ Phase D |

**Total**: 2,044 lines → Port to `commonTest` (largest and most complex test!)

---

#### **Category 5: Input & Touch Events** (3 tests) - ⭐⭐ MEDIUM PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveMultitouchTest.kt` | ~120 | Multi-touch handling | ❌ Platform-specific (Phase F) |
| `RiveNestedInputsTest.kt` | ~100 | Nested input handling | ✅ Phase E |
| `RiveStateMachineTouchEventTest.kt` | ~150 | Touch events (duplicate) | ❌ Already counted in Category 3 |

**Total**: ~220 lines (100 to commonTest, 120 to platformTest)

---

#### **Category 6: Events** (1 test) - ⭐⭐ MEDIUM PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveEventTest.kt` | ~180 | Event firing, event listeners | ✅ Phase C |

**Total**: ~180 lines → Port to `commonTest`

---

#### **Category 7: Assets** (1 test) - ⭐⭐ MEDIUM PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveAssetsTest.kt` | ~200 | Asset loading, image decoding, audio | ✅ Phase E |

**Total**: ~200 lines → Port to `commonTest`

---

#### **Category 8: Fonts** (2 tests) - ⭐ LOW PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `FontHelpersTest.kt` | ~150 | Font loading, font fallback | ❌ Phase 8+ (lower priority) |
| `FontPickerTest.kt` | ~100 | Font selection | ❌ Phase 8+ (lower priority) |

**Total**: ~250 lines → **Deferred** to Phase 8+

---

#### **Category 9: CommandQueue** (1 test) - ⭐⭐⭐ CRITICAL

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `CommandQueueComposeTest.kt` | ~50 | Reference counting, lifecycle, disposal | ✅ Phase A |

**Total**: ~50 lines → Port to `commonTest` (+ 200 lines of new CommandQueue tests)

---

#### **Category 10: Lifecycle & Memory** (4 tests) - ⭐⭐⭐ HIGH PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveLifecycleTest.kt` | ~200 | Component lifecycle | ✅ Phase G |
| `RiveViewLifecycleTest.kt` | ~180 | View lifecycle | ❌ Android View-specific (skip) |
| `RiveMemoryTests.kt` | ~250 | Memory leaks, resource cleanup | ✅ Phase E |
| `RiveControllerTest.kt` | ~150 | Controller lifecycle | ✅ Phase G |

**Total**: ~780 lines (600 to commonTest, 180 skipped)

---

#### **Category 11: UI Components** (4 tests) - ⭐ LOW PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveViewTest.kt` | ~300 | RiveAnimationView testing | ❌ Skip (Android View-specific) |
| `RiveViewStateMachineTest.kt` | ~200 | View + state machine | ❌ Skip (Android View-specific) |
| `RiveArtboardVolumeTest.kt` | ~100 | Volume control | ❌ Skip (Android View-specific) |
| `RiveTextValueRunTest.kt` | ~150 | Text run manipulation | ❌ Skip (Android View-specific) |

**Total**: ~750 lines → **Skipped** (not Compose)

---

#### **Category 12: Visual/Snapshot Tests** (1 test) - ⭐⭐ MEDIUM PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `SnapshotTest.kt` | ~300 | Visual regression testing | ❌ Phase 8+ (lower priority) |

**Total**: ~300 lines → **Deferred** to Phase 8+

---

#### **Category 13: Utilities** (2 tests) - ⭐ LOW PRIORITY

| Test File | Lines | What It Tests | Port to mprive? |
|-----------|-------|---------------|-----------------|
| `RiveUtilTest.kt` | ~100 | Utility functions | ✅ Phase G |
| `RiveListenerTest.kt` | ~80 | Event listener patterns | ✅ Phase C |

**Total**: ~180 lines → Port to `commonTest`

---

### Summary Statistics

| Category | Tests | Lines | Port to commonTest | Platform-specific | Skip/Defer |
|----------|-------|-------|-------------------|-------------------|------------|
| **High Priority** | 11 | ~1,530 | ✅ 1,530 | - | - |
| **Critical** | 2 | ~2,094 | ✅ 2,094 | - | - |
| **Medium Priority** | 8 | ~1,130 | ✅ 680 | 270 | 180 |
| **Low Priority** | 10 | ~1,480 | ✅ 180 | - | 1,300 |
| **TOTAL** | **31** | **~6,234** | **4,484** | **270** | **1,480** |

**Porting Rate**: ~72% of code (4,484 / 6,234 lines)

---

## Multiplatform Test Structure

### Directory Layout

```
mprive/src/
├── commonTest/                           # SHARED TESTS (run on all platforms)
│   ├── kotlin/app/rive/mp/test/
│   │   ├── utils/
│   │   │   ├── MpTestResources.kt        # Resource loading abstraction
│   │   │   ├── MpTestUtils.kt            # Common test utilities
│   │   │   ├── MpTestContext.kt          # Platform-agnostic test context
│   │   │   └── MpComposeTestUtils.kt     # Compose test helpers
│   │   ├── core/
│   │   │   ├── MpRiveFileLoadTest.kt     # ✅ Port from RiveFileLoadTest
│   │   │   ├── MpRiveArtboardLoadTest.kt
│   │   │   ├── MpRiveAnimationLoadTest.kt
│   │   │   ├── MpRiveStateMachineLoadTest.kt
│   │   │   ├── MpRiveDataBindingTest.kt  # ✅ 2,044 lines!
│   │   │   ├── MpRiveEventTest.kt
│   │   │   ├── MpRiveAssetsTest.kt
│   │   │   └── MpRiveMemoryTest.kt
│   │   └── commandqueue/
│   │       ├── MpCommandQueueTest.kt             # ✅ Port from CommandQueueComposeTest
│   │       ├── MpCommandQueueLifecycleTest.kt    # ✅ New (lifecycle management)
│   │       ├── MpCommandQueueThreadSafetyTest.kt # ✅ New (thread safety)
│   │       └── MpCommandQueueHandleTest.kt       # ✅ New (handle management)
│   └── resources/                        # SHARED TEST RESOURCES
│       └── rive/                         # (Copied as needed per phase)
│           ├── data_bind_test_impl.riv
│           ├── flux_capacitor.riv
│           ├── off_road_car_blog.riv
│           └── ... (copy as needed)
│
├── androidInstrumentedTest/              # ANDROID-SPECIFIC TESTS
│   ├── kotlin/app/rive/mp/test/
│   │   ├── platform/
│   │   │   ├── AndroidResourceLoaderTest.kt      # Test Android R.raw loading
│   │   │   ├── AndroidRenderContextTest.kt       # Test PLS renderer
│   │   │   └── AndroidLifecycleTest.kt           # Test Android lifecycle
│   │   ├── compose/
│   │   │   ├── ComposeAndroidIntegrationTest.kt
│   │   │   └── ComposeAndroidLifecycleTest.kt
│   │   └── ui/
│   │       ├── TouchEventTest.kt                 # Touch events (Android-specific)
│   │       └── MultitouchTest.kt                 # Multi-touch (Android-specific)
│   └── res/raw/                          # Android test resources (if needed)
│
└── desktopTest/                          # DESKTOP-SPECIFIC TESTS
    ├── kotlin/app/rive/mp/test/
    │   ├── platform/
    │   │   ├── DesktopResourceLoaderTest.kt      # Test Desktop file loading
    │   │   ├── DesktopRenderContextTest.kt       # Test Skia renderer
    │   │   └── DesktopLifecycleTest.kt           # Test Desktop lifecycle
    │   └── compose/
    │       ├── ComposeDesktopIntegrationTest.kt
    │       └── ComposeDesktopLifecycleTest.kt
    └── resources/                        # Desktop test resources (if needed)
```

---

## Resource Management

### Test Resources (37 files from kotlin/src/androidTest/res/raw/)

#### **Rive Animation Files** (32 .riv files):

| Resource File | Purpose | Phases Needed |
|---------------|---------|---------------|
| `data_bind_test_impl.riv` | **View model tests** (CRITICAL) | D, E |
| `flux_capacitor.riv` | Basic animation | B |
| `off_road_car_blog.riv` | Multiple animations | B, C |
| `multiple_animations.riv` | Animation queries | B |
| `multiple_state_machines.riv` | State machine tests | C |
| `state_machine_configurations.riv` | SM configurations | C |
| `events_test.riv` | Event firing | C |
| `layerstatechange.riv` | State transitions | C |
| `nested_settle.riv` | Nested state machines | C |
| `asset_load_check.riv` | Asset loading | E |
| `walle.riv` | Asset with embedded image | E |
| `cdn_image.riv` | CDN asset loading | B |
| `audio_test.riv` | Audio assets | E |
| `hello_world_text.riv` | Text runs | Phase 8+ |
| `runtime_nested_text_runs.riv` | Nested text | Phase 8+ |
| `multipleartboards.riv` | Artboard queries | B |
| `noartboard.riv` | Error case (no artboard) | B |
| `noanimation.riv` | Error case (no animation) | B |
| `junk.riv` | Error case (malformed file) | B |
| `sample6.riv` | Error case (unsupported version) | B |
| `multitouch.riv` | Multi-touch events | F (platform) |
| `touchevents.riv` | Touch events | F (platform) |
| `touchpassthrough.riv` | Touch passthrough | F (platform) |
| `nested_inputs_test.riv` | Nested inputs | E |
| `animationconfigurations.riv` | Animation configs | E |
| `blend_state.riv` | Blend states | C |
| `empty_animation_state.riv` | Empty state | C |
| `long_artboard_name.riv` | Edge case (long name) | B |
| `shapes.riv` | Basic shapes | B |
| `snapshot_test.riv` | Visual regression | Phase 8+ |
| `what_a_state.riv` | State machine | C |
| `style_fallback_fonts.riv` | Font fallback | Phase 8+ |

#### **Asset Files** (5 files):

| Asset File | Purpose | Phases Needed |
|------------|---------|---------------|
| `eve.png` | Image asset for tests | E |
| `font.ttf` | Font asset | Phase 8+ |
| `inter_24pt_regular_abcdef.ttf` | Font asset | Phase 8+ |
| `inter_extralight_onlya.ttf` | Font asset | Phase 8+ |
| `table.wav` | Audio asset | E |

#### **Configuration Files** (3 .xml files):

| Config File | Purpose | Phases Needed |
|-------------|---------|---------------|
| `fallback_fonts.xml` | Font fallback config | Phase 8+ |
| `fonts.xml` | Font config | Phase 8+ |
| `system_fonts.xml` | System font config | Phase 8+ |

---

### Resource Loading Strategy

**Phase-by-Phase Resource Copying:**

| Phase | Resources to Copy | Count |
|-------|-------------------|-------|
| **Phase A** | None (CommandQueue infrastructure only) | 0 |
| **Phase B** | File/artboard test files | 12 |
| **Phase C** | State machine test files | 10 |
| **Phase D** | `data_bind_test_impl.riv` | 1 |
| **Phase E** | Asset test files | 8 |
| **Phase F** | Platform-specific files | 3 |
| **Phase 8+** | Font/snapshot files | 3 |
| **TOTAL** | | 37 |

---

## Test Utilities

### 1. Resource Loading Abstraction

**File**: `mprive/src/commonTest/kotlin/app/rive/mp/test/utils/MpTestResources.kt`

```kotlin
package app.rive.mp.test.utils

/**
 * Platform-agnostic test resource loader.
 * 
 * Abstracts the difference between Android (R.raw) and Desktop (file paths).
 */
expect object MpTestResources {
    /**
     * Load a Rive animation file by name (without extension).
     * Example: loadRiveFile("flux_capacitor") loads "flux_capacitor.riv"
     */
    fun loadRiveFile(name: String): ByteArray
    
    /**
     * Load an image asset by name (without extension).
     * Example: loadImage("eve") loads "eve.png"
     */
    fun loadImage(name: String): ByteArray
    
    /**
     * Load an audio asset by name (without extension).
     */
    fun loadAudio(name: String): ByteArray
    
    /**
     * Load a font asset by name (without extension).
     */
    fun loadFont(name: String): ByteArray
}
```

**Android Implementation**: `mprive/src/androidInstrumentedTest/kotlin/app/rive/mp/test/utils/MpTestResources.android.kt`

```kotlin
package app.rive.mp.test.utils

import androidx.test.platform.app.InstrumentationRegistry
import app.rive.mp.test.R

actual object MpTestResources {
    private val context by lazy {
        InstrumentationRegistry.getInstrumentation().targetContext
    }
    
    private val resourceMap = mapOf(
        // Rive files
        "data_bind_test_impl" to R.raw.data_bind_test_impl,
        "flux_capacitor" to R.raw.flux_capacitor,
        "off_road_car_blog" to R.raw.off_road_car_blog,
        // ... add all files as needed
        
        // Images
        "eve" to R.raw.eve,
        
        // Audio
        "table" to R.raw.table,
        
        // Fonts
        "font" to R.raw.font,
        // ... add more as needed
    )
    
    actual fun loadRiveFile(name: String): ByteArray {
        val resId = resourceMap[name] 
            ?: throw IllegalArgumentException("Rive file not found: $name")
        return context.resources.openRawResource(resId).readBytes()
    }
    
    actual fun loadImage(name: String): ByteArray {
        val resId = resourceMap[name] 
            ?: throw IllegalArgumentException("Image not found: $name")
        return context.resources.openRawResource(resId).readBytes()
    }
    
    actual fun loadAudio(name: String): ByteArray {
        val resId = resourceMap[name] 
            ?: throw IllegalArgumentException("Audio not found: $name")
        return context.resources.openRawResource(resId).readBytes()
    }
    
    actual fun loadFont(name: String): ByteArray {
        val resId = resourceMap[name] 
            ?: throw IllegalArgumentException("Font not found: $name")
        return context.resources.openRawResource(resId).readBytes()
    }
}
```

**Desktop Implementation**: `mprive/src/desktopTest/kotlin/app/rive/mp/test/utils/MpTestResources.desktop.kt`

```kotlin
package app.rive.mp.test.utils

actual object MpTestResources {
    actual fun loadRiveFile(name: String): ByteArray {
        return loadResource("/rive/$name.riv")
    }
    
    actual fun loadImage(name: String): ByteArray {
        return loadResource("/rive/$name.png")
    }
    
    actual fun loadAudio(name: String): ByteArray {
        return loadResource("/rive/$name.wav")
    }
    
    actual fun loadFont(name: String): ByteArray {
        return loadResource("/rive/$name.ttf")
    }
    
    private fun loadResource(path: String): ByteArray {
        return this::class.java.getResourceAsStream(path)?.readBytes()
            ?: throw IllegalArgumentException("Resource not found: $path")
    }
}
```

---

### 2. Test Context Abstraction

**File**: `mprive/src/commonTest/kotlin/app/rive/mp/test/utils/MpTestContext.kt`

```kotlin
package app.rive.mp.test.utils

import app.rive.mp.RiveNative

/**
 * Platform-agnostic test context for initializing Rive.
 */
expect class MpTestContext() {
    /**
     * Initialize Rive runtime for testing.
     * On Android, this calls Rive.init(context).
     * On Desktop, this may be a no-op or minimal setup.
     */
    fun initRive()
    
    /**
     * Get platform name for debugging.
     */
    fun getPlatformName(): String
}

/**
 * Common test utilities.
 */
object MpTestUtils {
    /**
     * Wait until a condition is met or timeout.
     * Useful for async operations.
     */
    fun waitUntil(
        timeoutMs: Long = 5000,
        intervalMs: Long = 50,
        condition: () -> Boolean
    ) {
        var elapsed = 0L
        while (elapsed < timeoutMs) {
            if (condition()) return
            Thread.sleep(intervalMs)
            elapsed += intervalMs
        }
        throw AssertionError("Timeout waiting for condition after ${timeoutMs}ms")
    }
}
```

**Android Implementation**: `mprive/src/androidInstrumentedTest/kotlin/app/rive/mp/test/utils/MpTestContext.android.kt`

```kotlin
package app.rive.mp.test.utils

import androidx.test.platform.app.InstrumentationRegistry
import app.rive.runtime.kotlin.core.Rive

actual class MpTestContext {
    private val context by lazy {
        InstrumentationRegistry.getInstrumentation().targetContext
    }
    
    actual fun initRive() {
        Rive.init(context)
    }
    
    actual fun getPlatformName(): String = "Android"
}
```

**Desktop Implementation**: `mprive/src/desktopTest/kotlin/app/rive/mp/test/utils/MpTestContext.desktop.kt`

```kotlin
package app.rive.mp.test.utils

import app.rive.mp.RiveNative

actual class MpTestContext {
    actual fun initRive() {
        // Desktop doesn't need context-based initialization
        // But we can call nativeInit if needed
        RiveNative.nativeInit(Unit)
    }
    
    actual fun getPlatformName(): String = "Desktop"
}
```

---

### 3. Compose Test Utilities

**File**: `mprive/src/commonTest/kotlin/app/rive/mp/test/utils/MpComposeTestUtils.kt`

```kotlin
package app.rive.mp.test.utils

import androidx.compose.runtime.Composable

/**
 * Platform-agnostic Compose test utilities.
 */
expect class MpComposeTestRule {
    fun setContent(content: @Composable () -> Unit)
    fun runOnUiThread(block: () -> Unit)
    fun advanceTimeByFrame()
    fun advanceTimeBy(milliseconds: Long)
}

expect fun createMpComposeTestRule(): MpComposeTestRule
```

**Android Implementation**: `mprive/src/androidInstrumentedTest/kotlin/app/rive/mp/test/utils/MpComposeTestUtils.android.kt`

```kotlin
package app.rive.mp.test.utils

import androidx.activity.ComponentActivity
import androidx.compose.runtime.Composable
import androidx.compose.ui.test.junit4.createAndroidComposeRule

actual class MpComposeTestRule(
    private val androidRule: androidx.compose.ui.test.junit4.AndroidComposeTestRule<*, *>
) {
    actual fun setContent(content: @Composable () -> Unit) {
        androidRule.setContent(content)
    }
    
    actual fun runOnUiThread(block: () -> Unit) {
        androidRule.runOnUiThread(block)
    }
    
    actual fun advanceTimeByFrame() {
        androidRule.mainClock.advanceTimeByFrame()
    }
    
    actual fun advanceTimeBy(milliseconds: Long) {
        androidRule.mainClock.advanceTimeBy(milliseconds)
    }
}

actual fun createMpComposeTestRule(): MpComposeTestRule {
    return MpComposeTestRule(createAndroidComposeRule<ComponentActivity>())
}
```

**Desktop Implementation**: `mprive/src/desktopTest/kotlin/app/rive/mp/test/utils/MpComposeTestUtils.desktop.kt`

```kotlin
package app.rive.mp.test.utils

import androidx.compose.runtime.Composable
import androidx.compose.ui.test.junit4.createComposeRule

actual class MpComposeTestRule(
    private val desktopRule: androidx.compose.ui.test.junit4.ComposeContentTestRule
) {
    actual fun setContent(content: @Composable () -> Unit) {
        desktopRule.setContent(content)
    }
    
    actual fun runOnUiThread(block: () -> Unit) {
        desktopRule.runOnUiThread(block)
    }
    
    actual fun advanceTimeByFrame() {
        desktopRule.mainClock.advanceTimeByFrame()
    }
    
    actual fun advanceTimeBy(milliseconds: Long) {
        desktopRule.mainClock.advanceTimeBy(milliseconds)
    }
}

actual fun createMpComposeTestRule(): MpComposeTestRule {
    return MpComposeTestRule(createComposeRule())
}
```

---

## Test Porting Plan by Phase

### **Phase A: CommandQueue Foundation** (Week 1-2)

#### Tests to Port

**1. MpCommandQueueTest.kt** (from `CommandQueueComposeTest.kt`)

```kotlin
package app.rive.mp.test.commandqueue

import app.rive.mp.test.utils.*
import kotlin.test.*

@OptIn(ExperimentalRiveComposeAPI::class)
class MpCommandQueueTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun releases_when_removed_from_tree() {
        val composeRule = createMpComposeTestRule()
        
        lateinit var queue: CommandQueue
        lateinit var show: MutableState<Boolean>
        
        composeRule.setContent {
            show = remember { mutableStateOf(true) }
            if (show.value) {
                queue = rememberCommandQueue(autoPoll = false)
            }
        }
        
        assertEquals(1, queue.refCount)
        assertFalse(queue.isDisposed)
        
        // Remove from the Compose tree
        composeRule.runOnUiThread { show.value = false }
        composeRule.advanceTimeByFrame()
        
        assertEquals(0, queue.refCount)
        assertTrue(queue.isDisposed)
    }
}
```

#### New Tests to Create

**2. MpCommandQueueLifecycleTest.kt** (NEW)

```kotlin
package app.rive.mp.test.commandqueue

import app.rive.mp.CommandQueue
import app.rive.mp.test.utils.*
import kotlin.test.*

class MpCommandQueueLifecycleTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun constructor_increments_refCount() {
        val queue = CommandQueue()
        assertEquals(1, queue.refCount, "CommandQueue should start with refCount = 1")
        queue.release("test")
    }
    
    @Test
    fun acquire_increments_refCount() {
        val queue = CommandQueue()
        queue.acquire("test-owner")
        assertEquals(2, queue.refCount)
        queue.release("test-owner")
        queue.release("constructor")
    }
    
    @Test
    fun release_decrements_refCount() {
        val queue = CommandQueue()
        queue.acquire("test-owner")
        queue.release("test-owner")
        assertEquals(1, queue.refCount)
        queue.release("constructor")
    }
    
    @Test
    fun release_to_zero_disposes() {
        val queue = CommandQueue()
        assertFalse(queue.isDisposed)
        queue.release("constructor")
        assertTrue(queue.isDisposed)
        assertEquals(0, queue.refCount)
    }
    
    @Test
    fun double_release_throws() {
        val queue = CommandQueue()
        queue.release("constructor")
        assertFailsWith<IllegalStateException> {
            queue.release("invalid")
        }
    }
}
```

**3. MpCommandQueueThreadSafetyTest.kt** (NEW)

```kotlin
package app.rive.mp.test.commandqueue

import app.rive.mp.CommandQueue
import app.rive.mp.test.utils.*
import kotlinx.coroutines.*
import kotlin.test.*

class MpCommandQueueThreadSafetyTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun concurrent_acquire_release_is_safe() = runBlocking {
        val queue = CommandQueue()
        val iterations = 1000
        
        // Launch multiple coroutines that concurrently acquire and release
        val jobs = (1..10).map { threadId ->
            launch(Dispatchers.Default) {
                repeat(iterations) {
                    queue.acquire("thread-$threadId")
                    yield() // Encourage interleaving
                    queue.release("thread-$threadId")
                }
            }
        }
        
        jobs.joinAll()
        
        // Should end up back at refCount = 1 (constructor)
        assertEquals(1, queue.refCount)
        queue.release("constructor")
    }
}
```

**4. MpCommandQueueHandleTest.kt** (NEW)

```kotlin
package app.rive.mp.test.commandqueue

import app.rive.mp.CommandQueue
import app.rive.mp.test.utils.*
import kotlinx.coroutines.*
import kotlin.test.*

class MpCommandQueueHandleTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun loadFile_returns_unique_handles() = runBlocking {
        val queue = CommandQueue()
        val bytes1 = MpTestResources.loadRiveFile("flux_capacitor")
        val bytes2 = MpTestResources.loadRiveFile("off_road_car_blog")
        
        val handle1 = queue.loadFile(bytes1)
        val handle2 = queue.loadFile(bytes2)
        
        assertNotEquals(handle1.handle, handle2.handle, "Handles should be unique")
        
        queue.deleteFile(handle1)
        queue.deleteFile(handle2)
        queue.release("constructor")
    }
    
    @Test
    fun handles_are_incrementing() = runBlocking {
        val queue = CommandQueue()
        val bytes = MpTestResources.loadRiveFile("flux_capacitor")
        
        val handle1 = queue.loadFile(bytes)
        val handle2 = queue.loadFile(bytes)
        val handle3 = queue.loadFile(bytes)
        
        assertTrue(handle2.handle > handle1.handle)
        assertTrue(handle3.handle > handle2.handle)
        
        queue.deleteFile(handle1)
        queue.deleteFile(handle2)
        queue.deleteFile(handle3)
        queue.release("constructor")
    }
}
```

#### Resources Needed
- None (CommandQueue tests don't use Rive files)

#### Coverage Target
- CommandQueue: 80%+

---

### **Phase B: File & Artboard Operations** (Week 2-3)

#### Tests to Port

**1. MpRiveFileLoadTest.kt** (from `RiveFileLoadTest.kt`)

```kotlin
package app.rive.mp.test.core

import app.rive.mp.RiveFile
import app.rive.mp.test.utils.*
import kotlin.test.*

class MpRiveFileLoadTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun loadFormatFlux() {
        val bytes = MpTestResources.loadRiveFile("flux_capacitor")
        val file = RiveFile.load(bytes)
        assertEquals(1, file.artboardCount)
        file.dispose()
    }
    
    @Test
    fun loadFormatBuggy() {
        val bytes = MpTestResources.loadRiveFile("off_road_car_blog")
        val file = RiveFile.load(bytes)
        assertEquals(5, file.artboardCount)
        file.dispose()
    }
    
    @Test
    fun loadJunk_throws() {
        val bytes = MpTestResources.loadRiveFile("junk")
        assertFailsWith<MalformedFileException> {
            RiveFile.load(bytes)
        }
    }
    
    @Test
    fun loadUnsupportedVersion_throws() {
        val bytes = MpTestResources.loadRiveFile("sample6")
        assertFailsWith<UnsupportedRuntimeVersionException> {
            RiveFile.load(bytes)
        }
    }
}
```

**2. MpRiveArtboardLoadTest.kt** (from `RiveArtboardLoadTest.kt`)

```kotlin
package app.rive.mp.test.core

import app.rive.mp.RiveFile
import app.rive.mp.test.utils.*
import kotlin.test.*

class MpRiveArtboardLoadTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun getDefaultArtboard() {
        val bytes = MpTestResources.loadRiveFile("multipleartboards")
        val file = RiveFile.load(bytes)
        
        val artboard = file.artboard() // Default artboard
        assertNotNull(artboard)
        
        artboard.dispose()
        file.dispose()
    }
    
    @Test
    fun getArtboardByIndex() {
        val bytes = MpTestResources.loadRiveFile("multipleartboards")
        val file = RiveFile.load(bytes)
        
        assertTrue(file.artboardCount > 0)
        val artboard = file.artboard(0)
        assertNotNull(artboard)
        
        artboard.dispose()
        file.dispose()
    }
    
    @Test
    fun getArtboardByName() {
        val bytes = MpTestResources.loadRiveFile("multipleartboards")
        val file = RiveFile.load(bytes)
        
        val artboard = file.artboard("Artboard1") // Assuming this name exists
        assertNotNull(artboard)
        
        artboard.dispose()
        file.dispose()
    }
    
    @Test
    fun noArtboard_throws() {
        val bytes = MpTestResources.loadRiveFile("noartboard")
        assertFailsWith<RiveException> {
            RiveFile.load(bytes)
        }
    }
}
```

#### Resources to Copy
- `flux_capacitor.riv`
- `off_road_car_blog.riv`
- `junk.riv`
- `sample6.riv`
- `multipleartboards.riv`
- `noartboard.riv`
- `long_artboard_name.riv`
- `shapes.riv`
- `cdn_image.riv`
- `noanimation.riv`
- `walle.riv` (for asset loader tests)
- `eve.png` (for asset loader tests)

**Total**: 12 files → Copy to `commonTest/resources/rive/`

#### Coverage Target
- File loading: 90%+
- Artboard operations: 90%+

---

### **Phase C: State Machines & Rendering** (Week 3-4)

#### Tests to Port

**1. MpRiveStateMachineLoadTest.kt**
**2. MpRiveStateMachineInstanceTest.kt**
**3. MpRiveStateMachineConfigurationsTest.kt**
**4. MpRiveEventTest.kt**
**5. MpRiveListenerTest.kt**

*(Similar structure to Phase B, ported from Android tests)*

#### Resources to Copy
- `multiple_state_machines.riv`
- `state_machine_configurations.riv`
- `state_machine_state_resolution.riv`
- `events_test.riv`
- `layerstatechange.riv`
- `nested_settle.riv`
- `what_a_state.riv`
- `blend_state.riv`
- `empty_animation_state.riv`
- `animationconfigurations.riv`

**Total**: 10 files → Copy to `commonTest/resources/rive/`

#### Coverage Target
- State machines: 85%+
- Events: 80%+

---

### **Phase D: View Models & Properties** (Week 4-5)

#### Tests to Port

**1. MpRiveDataBindingTest.kt** (from `RiveDataBindingTest.kt` - 2,044 lines!)

This is the **LARGEST and most CRITICAL** test. It covers:
- VM/VMI creation (default, blank, named, by index)
- All property types (number, string, boolean, enum, color, trigger, image, artboard)
- Nested properties
- Property flows (coroutines-based subscriptions)
- List properties (add, remove, swap, indexing)
- Bindable artboards
- Transfer mechanism (moving VMI between files)
- Concurrent access patterns
- Memory management

**Structure**:
```kotlin
package app.rive.mp.test.core

import app.rive.mp.test.utils.*
import kotlinx.coroutines.test.runTest
import kotlin.test.*

class MpRiveDataBindingTest {
    // 2,000+ lines of comprehensive view model tests
    // Direct port from Android with minimal changes
    // (Already uses coroutines, which are multiplatform-compatible!)
}
```

#### Resources to Copy
- `data_bind_test_impl.riv` (**CRITICAL** - main view model test file)

**Total**: 1 file → Copy to `commonTest/resources/rive/`

#### Coverage Target
- View models: 90%+ (most complex feature)

---

### **Phase E: Advanced Features** (Week 5-6)

#### Tests to Port

**1. MpRiveAssetsTest.kt**
**2. MpRiveAnimationConfigurationsTest.kt**
**3. MpRiveMemoryTest.kt**
**4. MpRiveNestedInputsTest.kt**

#### Resources to Copy
- `asset_load_check.riv`
- `audio_test.riv`
- `table.wav`
- `nested_inputs_test.riv`

**Total**: 4 files → Copy to `commonTest/resources/rive/`

#### Coverage Target
- Assets: 70%+
- Memory: 80%+

---

### **Phase F: Multiplatform Adaptation** (Week 6-7)

#### Platform-Specific Tests to Create

**Android** (`androidInstrumentedTest/platform/`):
1. `AndroidRenderContextTest.kt` - Test PLS renderer initialization, surface management
2. `AndroidResourceLoaderTest.kt` - Test R.raw resource loading
3. `AndroidLifecycleTest.kt` - Test Android lifecycle integration
4. `TouchEventTest.kt` - Touch event handling (from `RiveMultitouchTest.kt`)

**Desktop** (`desktopTest/platform/`):
1. `DesktopRenderContextTest.kt` - Test Skia renderer initialization
2. `DesktopResourceLoaderTest.kt` - Test file resource loading
3. `DesktopLifecycleTest.kt` - Test Desktop lifecycle integration

#### Resources to Copy
- `multitouch.riv`
- `touchevents.riv`
- `touchpassthrough.riv`

**Total**: 3 files → Copy to platform-specific test resources

#### Coverage Target
- Platform-specific: 80%+

---

### **Phase G: Testing & Optimization** (Week 7)

#### Tests to Port

**1. MpRiveLifecycleTest.kt**
**2. MpRiveControllerTest.kt**
**3. MpRiveUtilTest.kt**

#### Integration Tests

Create new integration tests that combine multiple components:
- Full end-to-end CommandQueue + view model workflow
- Performance benchmarks
- Memory leak detection (long-running tests)

#### Coverage Target
- Overall: 85%+

---

## Test Execution

### Running Tests

**Android**:
```bash
# Run all Android tests (common + androidInstrumentedTest)
./gradlew :mprive:connectedAndroidTest

# Run only common tests on Android
./gradlew :mprive:connectedAndroidTest --tests "app.rive.mp.test.*"

# Run only platform-specific tests
./gradlew :mprive:connectedAndroidTest --tests "*.platform.*"
```

**Desktop**:
```bash
# Run all Desktop tests (common + desktopTest)
./gradlew :mprive:desktopTest

# Run only common tests on Desktop
./gradlew :mprive:desktopTest --tests "app.rive.mp.test.*"

# Run only platform-specific tests
./gradlew :mprive:desktopTest --tests "*.platform.*"
```

**Both Platforms**:
```bash
# Run common tests on both platforms
./gradlew :mprive:connectedAndroidTest :mprive:desktopTest
```

---

## Coverage Targets

### Overall Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| **Line Coverage** | 85%+ | JaCoCo / Kover |
| **Branch Coverage** | 75%+ | JaCoCo / Kover |
| **Critical Path Coverage** | 95%+ | Manual verification |

### Per-Phase Coverage

| Phase | Component | Target Coverage | Critical Tests |
|-------|-----------|-----------------|----------------|
| **A** | CommandQueue | 80%+ | Lifecycle, thread safety, handles |
| **B** | File/Artboard | 90%+ | Loading, queries, error handling |
| **C** | State Machines | 85%+ | SM operations, events, advancement |
| **D** | View Models | 90%+ | Properties, flows, lists, transfer |
| **E** | Assets | 70%+ | Images, audio, fonts |
| **F** | Platform | 80%+ | Renderers, resource loading |
| **G** | Integration | 85%+ | End-to-end workflows |

### Test Count Summary

| Category | Tests to Port | New Tests | Platform Tests | Total |
|----------|---------------|-----------|----------------|-------|
| **Phase A** | 1 | 3 | 0 | 4 |
| **Phase B** | 2 | 0 | 0 | 2 |
| **Phase C** | 5 | 0 | 0 | 5 |
| **Phase D** | 1 | 0 | 0 | 1 |
| **Phase E** | 4 | 0 | 0 | 4 |
| **Phase F** | 0 | 0 | 7 | 7 |
| **Phase G** | 3 | 2 | 0 | 5 |
| **TOTAL** | **16** | **5** | **7** | **28** |

**Porting Rate**: 16 / 31 = ~52% of Android tests (covering ~72% of code)

---

## Success Criteria

### Phase Completion

Each phase is considered complete when:
1. ✅ All tests ported and passing on **both Android and Desktop**
2. ✅ Coverage targets met
3. ✅ No memory leaks detected
4. ✅ Performance benchmarks within targets
5. ✅ Documentation updated

### Overall Success

mprive testing is complete when:
1. ✅ **28 tests** implemented (16 ported + 5 new + 7 platform)
2. ✅ **85%+ overall code coverage**
3. ✅ **All tests pass** on Android and Desktop
4. ✅ **Zero memory leaks** in continuous tests
5. ✅ **Performance parity** with kotlin module
6. ✅ **Test documentation** complete

---

## Next Steps

1. ✅ **User approval** of this testing strategy ✅ APPROVED
2. **Toggle to Act mode** to create test structure
3. **Create directory structure** (commonTest, androidTest, desktopTest)
4. **Implement test utilities** (MpTestResources, MpTestContext, MpComposeTestUtils)
5. **Begin Phase A** (CommandQueue tests)
6. **Incrementally port tests** phase by phase
7. **Measure coverage** after each phase
8. **Iterate and improve** based on results

---

**End of mprive Testing Strategy**
