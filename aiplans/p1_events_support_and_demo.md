# Rive Events Support and Events Demo Implementation Plan

## Implementation Progress

| Phase | Description | Status |
|-------|-------------|--------|
| Phase 1 | C++ Layer - Event Retrieval Commands | ⏳ Not Started |
| Phase 2 | JNI/Bindings Layer | ⏳ Not Started |
| Phase 3 | Kotlin/Common Layer - Event Classes | ✅ Complete |
| Phase 4 | CommandQueue Event Methods | ⏳ Not Started |
| Phase 5 | StateMachine Event Support | ⏳ Not Started |
| Phase 6 | Rive Composable - onEvent Parameter | ⏳ Not Started |
| Phase 7 | Demo App Implementation | ⏳ Not Started |

**Last Updated:** 2026-01-21

---

## Overview

This document outlines the plan to implement Rive Events support in the `mprive` multiplatform library and create an Events demo screen in the `mpapp` module, matching the functionality of the reference implementation in `app/src/main/java/app/rive/runtime/example/EventsActivity.kt`.

## Reference Implementation Analysis

### Event Classes in rive-android (kotlin module)

The reference implementation uses the following event classes:

1. **`RiveEvent`** (base class) - `kotlin/src/main/java/app/rive/runtime/kotlin/core/RiveEvent.kt`
   - Properties: `name`, `type`, `delay`, `properties`, `data`
   - Uses JNI to access native C++ Event via pointer

2. **`RiveGeneralEvent`** (extends RiveEvent) - `kotlin/src/main/java/app/rive/runtime/kotlin/core/RiveGeneralEvent.kt`
   - General-purpose event with custom properties
   - TypeKey: 128

3. **`RiveOpenURLEvent`** (extends RiveEvent) - `kotlin/src/main/java/app/rive/runtime/kotlin/core/RiveOpenURLEvent.kt`
   - URL event with `url` and `target` properties
   - TypeKey: 131

4. **`EventType`** enum - `kotlin/src/main/java/app/rive/runtime/kotlin/core/EventType.kt`
   - `GeneralEvent(128)`, `OpenURLEvent(131)`

5. **`RiveEventReport`** - Wraps event pointer + delay, converts to typed event

### Event Flow in Reference Implementation

1. **Native Layer**: `StateMachineInstance` tracks events via `reportedEventCount()` and `reportedEventAt(index)`
2. **JNI Bridge**: `StateMachineInstance.kt` has:
   - `cppReportedEventCount(cppPointer: Long): Int`
   - `cppReportedEventAt(cppPointer: Long, index: Int): RiveEventReport`
3. **Event Polling**: In `RiveFileController.advance()`:
   ```kotlin
   if (eventListeners.isNotEmpty()) {
       stateMachineInstance.eventsReported.forEach {
           notifyEvent(it)
       }
   }
   ```
4. **Event Dispatch**: `RiveFileController` maintains:
   - `_eventListeners: MutableSet<RiveEventListener>`
   - `addEventListener(listener)` / `removeEventListener(listener)`
   - `notifyEvent(event: RiveEvent)` dispatches to all listeners
5. **UI Usage**: `RiveAnimationView.addEventListener()` delegates to controller

### Rive C++ Runtime Event API

From `submodules/rive-runtime/include/rive/`:

- **`EventReport`**: Holds `Event*` pointer and `secondsDelay`
- **`Event`**: Base class (typeKey 128) extending `CustomPropertyGroup`
- **`OpenUrlEvent`**: Extends Event with `url()` and `targetValue()`
- **`StateMachineInstance`**:
  - `reportedEventCount(): size_t`
  - `reportedEventAt(index): const EventReport`

---

## API Design Decision

**Chosen approach**: Compose-idiomatic **callback pattern** (`onEvent: ((RiveEvent) -> Unit)?`)

This differs from the reference implementation's listener pattern but is more natural for Compose Multiplatform. The event classes themselves will match the reference API.

### Comparison

| Aspect | Reference (kotlin/src/main) | mprive API |
|--------|---------------------------|--------------------|
| Event delivery | Listener pattern (`addEventListener`) | Callback parameter (`onEvent`) |
| Event classes | JNI-backed with `cppPointer` | Pure Kotlin data classes |
| Registration | Mutable listener set | Composable recomposition |
| Multiplatform | Android only | KMP (Android, Desktop, iOS, Wasm) |

---

## Implementation Plan

### Phase 1: C++ Layer - Event Retrieval Commands

**Files to modify:**
- `mprive/src/nativeInterop/cpp/include/command_types.hpp`
- `mprive/src/nativeInterop/cpp/src/command_server/command_server_state_machine.cpp`
- `mprive/src/nativeInterop/cpp/include/command_server.hpp`

#### 1.1 Add New Command Types

Add to `command_types.hpp`:
```cpp
// Event Commands
GetReportedEventCount,      // Get count of events fired since last advance
GetReportedEventAt,         // Get event at index
```

#### 1.2 Add Event Data Structures

Create new file `mprive/src/nativeInterop/cpp/include/rive_event.hpp`:
```cpp
#ifndef RIVE_EVENT_DATA_HPP
#define RIVE_EVENT_DATA_HPP

#include <string>
#include <map>
#include <variant>
#include <vector>

namespace mprive {

enum class RiveEventType : uint16_t {
    GeneralEvent = 128,
    OpenURLEvent = 131
};

// Variant for property values
using PropertyValue = std::variant<
    bool,
    float,
    std::string
>;

// Data structure to pass event data to Kotlin
struct RiveEventData {
    std::string name;
    RiveEventType type;
    float delay;
    std::string url;           // For OpenURLEvent
    std::string target;        // For OpenURLEvent
    std::map<std::string, PropertyValue> properties;
};

} // namespace mprive

#endif
```

#### 1.3 Implement Event Retrieval in Command Server

Add to `command_server_state_machine.cpp`:
```cpp
#include "rive/event.hpp"
#include "rive/event_report.hpp"
#include "rive/open_url_event.hpp"
#include "rive/custom_property.hpp"

size_t CommandServer::getReportedEventCount(StateMachineHandle smHandle) {
    auto* sm = getStateMachine(smHandle);
    if (!sm) return 0;
    return sm->reportedEventCount();
}

RiveEventData CommandServer::getReportedEventAt(StateMachineHandle smHandle, size_t index) {
    auto* sm = getStateMachine(smHandle);
    if (!sm) return {};
    
    const rive::EventReport report = sm->reportedEventAt(index);
    rive::Event* event = report.event();
    if (!event) return {};
    
    RiveEventData data;
    data.name = event->name();
    data.delay = report.secondsDelay();
    data.type = static_cast<RiveEventType>(event->coreType());
    
    // Extract properties from CustomPropertyContainer
    extractEventProperties(event, data.properties);
    
    // For OpenURLEvent, extract URL and target
    if (event->coreType() == rive::OpenUrlEventBase::typeKey) {
        auto* urlEvent = static_cast<rive::OpenUrlEvent*>(event);
        data.url = urlEvent->url();
        data.target = urlEvent->targetValue();
    }
    
    return data;
}

// Helper to extract properties from event
void CommandServer::extractEventProperties(
    rive::Event* event, 
    std::map<std::string, PropertyValue>& properties
) {
    // Iterate through custom properties on the event
    // Based on CustomPropertyContainer interface
    for (auto* child : event->children()) {
        if (auto* boolProp = child->as<rive::CustomPropertyBoolean>()) {
            properties[boolProp->name()] = boolProp->propertyValue();
        } else if (auto* numProp = child->as<rive::CustomPropertyNumber>()) {
            properties[numProp->name()] = static_cast<float>(numProp->propertyValue());
        } else if (auto* strProp = child->as<rive::CustomPropertyString>()) {
            properties[strProp->name()] = std::string(strProp->propertyValue());
        }
    }
}
```

### Phase 2: JNI/Bindings Layer

**Files to modify:**
- `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_state_machine.cpp`

#### 2.1 Add JNI Functions for Event Retrieval

```cpp
extern "C" JNIEXPORT jint JNICALL
Java_app_rive_mp_core_CommandQueueBridgeKt_nativeGetReportedEventCount(
    JNIEnv* env,
    jclass clazz,
    jlong commandServerPtr,
    jlong smHandle
) {
    auto* server = reinterpret_cast<CommandServer*>(commandServerPtr);
    return static_cast<jint>(server->getReportedEventCount(
        StateMachineHandle{static_cast<uint64_t>(smHandle)}
    ));
}

// Returns event data as a jobject (RiveEventData)
extern "C" JNIEXPORT jobject JNICALL
Java_app_rive_mp_core_CommandQueueBridgeKt_nativeGetReportedEventAt(
    JNIEnv* env,
    jclass clazz,
    jlong commandServerPtr,
    jlong smHandle,
    jint index
) {
    auto* server = reinterpret_cast<CommandServer*>(commandServerPtr);
    RiveEventData eventData = server->getReportedEventAt(
        StateMachineHandle{static_cast<uint64_t>(smHandle)},
        static_cast<size_t>(index)
    );
    
    // Create Kotlin RiveEventData object
    return createRiveEventDataObject(env, eventData);
}

// Helper to create the Kotlin event data object
jobject createRiveEventDataObject(JNIEnv* env, const RiveEventData& data) {
    // Find the Kotlin class
    jclass eventDataClass = env->FindClass("app/rive/mp/event/RiveEventData");
    
    // Get constructor
    jmethodID constructor = env->GetMethodID(eventDataClass, "<init>",
        "(Ljava/lang/String;SFLjava/lang/String;Ljava/lang/String;Ljava/util/Map;)V");
    
    // Convert properties map
    jobject propertiesMap = createPropertiesMap(env, data.properties);
    
    // Create strings
    jstring name = env->NewStringUTF(data.name.c_str());
    jstring url = env->NewStringUTF(data.url.c_str());
    jstring target = env->NewStringUTF(data.target.c_str());
    
    // Create object
    return env->NewObject(eventDataClass, constructor,
        name,
        static_cast<jshort>(data.type),
        data.delay,
        url,
        target,
        propertiesMap
    );
}
```

### Phase 3: Kotlin/Common Layer - Event Classes ✅ COMPLETE

**Status:** Implemented on 2026-01-21

**Files created in `mprive/src/commonMain/kotlin/app/rive/mp/event/`:**

#### 3.1 EventType.kt

```kotlin
package app.rive.mp.event

/**
 * The type of Rive event.
 * 
 * Matches the reference implementation in kotlin/src/main.
 */
enum class EventType(val value: Short) {
    GeneralEvent(128),
    OpenURLEvent(131);

    companion object {
        private val map = entries.associateBy(EventType::value)
        fun fromValue(value: Short) = map[value] ?: GeneralEvent
    }
}
```

#### 3.2 RiveEvent.kt

```kotlin
package app.rive.mp.event

/**
 * A Rive event emitted by a state machine during animation playback.
 *
 * Events can carry custom properties and be used to trigger application logic
 * in response to animation state changes.
 *
 * This is a pure Kotlin data class (no native pointers) suitable for
 * Kotlin Multiplatform.
 *
 * @property name Name of the event as defined in Rive.
 * @property type Type of event ([EventType.GeneralEvent] or [EventType.OpenURLEvent]).
 * @property delay Delay in seconds since the advance that triggered the event.
 * @property properties Custom properties attached to the event.
 * @property data All event data as a flat map.
 *
 * @see RiveGeneralEvent
 * @see RiveOpenURLEvent
 */
open class RiveEvent(
    val name: String,
    val type: EventType,
    val delay: Float,
    val properties: Map<String, Any>,
    val data: Map<String, Any>
) {
    override fun toString(): String = "RiveEvent(name=$name, type=$type, delay=$delay)"
}
```

#### 3.3 RiveGeneralEvent.kt

```kotlin
package app.rive.mp.event

/**
 * A general-purpose Rive event.
 *
 * General events can carry arbitrary properties defined in the Rive editor.
 * Access properties via the [properties] map.
 *
 * Example:
 * ```kotlin
 * if (event is RiveGeneralEvent) {
 *     val rating = event.properties["rating"] as? Number
 *     println("Rating: $rating")
 * }
 * ```
 *
 * @see RiveOpenURLEvent
 */
class RiveGeneralEvent(
    name: String,
    delay: Float,
    properties: Map<String, Any>,
    data: Map<String, Any>
) : RiveEvent(name, EventType.GeneralEvent, delay, properties, data) {
    override fun toString(): String =
        "RiveGeneralEvent(name=$name, delay=$delay, properties=$properties)"
}
```

#### 3.4 RiveOpenURLEvent.kt

```kotlin
package app.rive.mp.event

/**
 * A Rive event that requests opening a URL.
 *
 * This event type is specifically designed for "Open URL" actions in Rive.
 * The application should handle this event by opening the URL in an appropriate way.
 *
 * Example:
 * ```kotlin
 * if (event is RiveOpenURLEvent) {
 *     uriHandler.openUri(event.url)
 * }
 * ```
 *
 * @property url The URL to open.
 * @property target The target window/context for the URL (e.g., "_blank", "_self").
 *
 * @see RiveGeneralEvent
 */
class RiveOpenURLEvent(
    name: String,
    delay: Float,
    properties: Map<String, Any>,
    data: Map<String, Any>,
    val url: String,
    val target: String
) : RiveEvent(name, EventType.OpenURLEvent, delay, properties, data) {
    override fun toString(): String =
        "RiveOpenURLEvent(name=$name, url=$url, target=$target)"
}
```

#### 3.5 RiveEventData.kt (Internal transfer class)

```kotlin
package app.rive.mp.event

/**
 * Internal data class for transferring event data from native layer.
 * Used by the JNI bindings to create typed event objects.
 */
internal data class RiveEventData(
    val name: String,
    val typeCode: Short,
    val delay: Float,
    val url: String,
    val target: String,
    val properties: Map<String, Any>
) {
    /**
     * Converts this data object to the appropriate RiveEvent subclass.
     */
    fun toRiveEvent(): RiveEvent {
        val type = EventType.fromValue(typeCode)
        val data = buildMap<String, Any> {
            put("name", name)
            put("type", type.value)
            put("delay", delay)
            putAll(properties)
            if (url.isNotEmpty()) put("url", url)
            if (target.isNotEmpty()) put("target", target)
        }
        
        return when (type) {
            EventType.OpenURLEvent -> RiveOpenURLEvent(
                name = name,
                delay = delay,
                properties = properties,
                data = data,
                url = url,
                target = target
            )
            EventType.GeneralEvent -> RiveGeneralEvent(
                name = name,
                delay = delay,
                properties = properties,
                data = data
            )
        }
    }
}
```

### Phase 4: CommandQueue Event Methods

**Files to modify:**
- `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt`
- `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt`

#### 4.1 Add to CommandQueueBridge.kt (expect declarations)

```kotlin
// Event retrieval
internal expect suspend fun getReportedEventCount(smHandle: StateMachineHandle): Int
internal expect suspend fun getReportedEventAt(smHandle: StateMachineHandle, index: Int): RiveEventData?
```

#### 4.2 Add to CommandQueue.kt

```kotlin
import app.rive.mp.event.RiveEvent
import app.rive.mp.event.RiveEventData

/**
 * Gets the number of events reported since the last state machine advance.
 *
 * @param smHandle The state machine handle.
 * @return The number of events.
 */
suspend fun getReportedEventCount(smHandle: StateMachineHandle): Int {
    return bridge.getReportedEventCount(smHandle)
}

/**
 * Gets an event at the specified index.
 *
 * @param smHandle The state machine handle.
 * @param index The event index (0 to count-1).
 * @return The event, or null if index is out of bounds.
 */
suspend fun getReportedEventAt(smHandle: StateMachineHandle, index: Int): RiveEvent? {
    return bridge.getReportedEventAt(smHandle, index)?.toRiveEvent()
}

/**
 * Polls all events from a state machine that occurred during the last advance.
 *
 * @param smHandle The state machine handle.
 * @return List of events (may be empty).
 */
suspend fun pollEvents(smHandle: StateMachineHandle): List<RiveEvent> {
    val count = getReportedEventCount(smHandle)
    if (count == 0) return emptyList()
    
    return (0 until count).mapNotNull { index ->
        getReportedEventAt(smHandle, index)
    }
}
```

### Phase 5: StateMachine Event Support

**Files to modify:**
- `mprive/src/commonMain/kotlin/app/rive/mp/StateMachine.kt`

#### 5.1 Add pollEvents method

```kotlin
import app.rive.mp.event.RiveEvent

/**
 * Polls and returns all events fired during the last advance.
 *
 * This should be called after [advance] to retrieve any events that were triggered.
 * Events are cleared after each advance, so call this before the next advance.
 *
 * Example:
 * ```kotlin
 * stateMachine.advance(deltaTime)
 * val events = stateMachine.pollEvents()
 * events.forEach { event ->
 *     when (event) {
 *         is RiveGeneralEvent -> handleGeneral(event)
 *         is RiveOpenURLEvent -> handleUrl(event)
 *     }
 * }
 * ```
 *
 * @return List of events fired during the last advance (may be empty).
 * @throws IllegalStateException If the state machine has been closed.
 */
suspend fun pollEvents(): List<RiveEvent> {
    check(!closed) { "StateMachine has been closed" }
    return riveWorker.pollEvents(stateMachineHandle)
}
```

### Phase 6: Rive Composable - onEvent Parameter

**Files to modify:**
- `mprive/src/commonMain/kotlin/app/rive/mp/compose/Rive.kt`
- `mprive/src/androidMain/kotlin/app/rive/mp/compose/Rive.android.kt`
- `mprive/src/desktopMain/kotlin/app/rive/mp/compose/Rive.desktop.kt`

#### 6.1 Update Rive.kt (expect declaration)

```kotlin
import app.rive.mp.event.RiveEvent

/**
 * The main composable for rendering a Rive file's artboard and state machine.
 *
 * ...existing documentation...
 *
 * @param onEvent Callback invoked when events are fired by the state machine.
 *    Events are polled after each frame advance. The callback receives each
 *    [RiveEvent] that was fired, allowing the application to respond to
 *    animation events.
 *
 *    Example:
 *    ```kotlin
 *    Rive(
 *        file = file,
 *        onEvent = { event ->
 *            when (event) {
 *                is RiveOpenURLEvent -> openUrl(event.url)
 *                is RiveGeneralEvent -> handleEvent(event)
 *            }
 *        }
 *    )
 *    ```
 *
 * ...existing params...
 */
@Composable
expect fun Rive(
    file: RiveFile,
    modifier: Modifier = Modifier,
    playing: Boolean = true,
    artboard: Artboard? = null,
    stateMachine: StateMachine? = null,
    viewModelInstance: ViewModelInstance? = null,
    fit: Fit = Fit.Contain(),
    backgroundColor: Int = Color.Transparent.toArgb(),
    pointerInputMode: RivePointerInputMode = RivePointerInputMode.Consume,
    onEvent: ((RiveEvent) -> Unit)? = null  // NEW PARAMETER
)
```

#### 6.2 Update Rive.android.kt

Add event polling to the render loop:

```kotlin
// In the LaunchedEffect render loop, after advancing state machine:

// Advance state machine if present
stateMachineToUse?.advance(deltaTime)

// Poll and dispatch events if callback is provided
if (onEvent != null && stateMachineToUse != null) {
    val events = stateMachineToUse.pollEvents()
    events.forEach { event ->
        onEvent(event)
    }
}

// Continue with animation advance and draw...
```

#### 6.3 Update Rive.desktop.kt

Add the parameter to the stub implementation:

```kotlin
@Composable
actual fun Rive(
    file: RiveFile,
    modifier: Modifier,
    playing: Boolean,
    artboard: Artboard?,
    stateMachine: StateMachine?,
    viewModelInstance: ViewModelInstance?,
    fit: Fit,
    backgroundColor: Int,
    pointerInputMode: RivePointerInputMode,
    onEvent: ((RiveEvent) -> Unit)?  // NEW PARAMETER
) {
    // Desktop stub - events not yet implemented
    Text("Rive Desktop - Coming Soon")
}
```

### Phase 7: Demo App Implementation

#### 7.1 Copy Required Assets

Copy from `app/src/main/res/raw/` to `mpapp/src/commonMain/composeResources/files/`:
- `url_event_button.riv`
- `log_event_button.riv`

#### 7.2 Update DemoRiveFiles.kt

```kotlin
enum class DemoRiveFile(val displayName: String, val resourcePath: String) {
    MARTY("Marty", "files/marty.riv"),
    OFF_ROAD_CAR("Off Road Car Blog", "files/off_road_car_blog.riv"),
    RATING("Rating", "files/rating.riv"),
    BASKETBALL("Basketball", "files/basketball.riv"),
    BUTTON("Button", "files/button.riv"),
    BLINKO("Blinko", "files/blinko.riv"),
    EXPLORER("Explorer", "files/explorer.riv"),
    F22("F22", "files/f22.riv"),
    MASCOT("Mascot", "files/mascot.riv"),
    // NEW: Event demo files
    URL_EVENT_BUTTON("URL Event Button", "files/url_event_button.riv"),
    LOG_EVENT_BUTTON("Log Event Button", "files/log_event_button.riv");
    
    // ... companion object unchanged
}
```

#### 7.3 Create EventsDemo.kt

Create `mpapp/src/commonMain/kotlin/app/rive/mpapp/EventsDemo.kt`:

```kotlin
package app.rive.mpapp

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.unit.dp
import app.rive.mp.Result
import app.rive.mp.compose.Fit
import app.rive.mp.compose.Rive
import app.rive.mp.compose.RiveFileSource
import app.rive.mp.compose.rememberRiveFile
import app.rive.mp.compose.rememberRiveWorker
import app.rive.mp.event.RiveEvent
import app.rive.mp.event.RiveGeneralEvent
import app.rive.mp.event.RiveOpenURLEvent

/**
 * Events demo screen showcasing Rive event handling.
 * 
 * Demonstrates three types of event handling:
 * 1. Star Rating - Reads custom property from general event
 * 2. URL Button - Opens URL from OpenURLEvent
 * 3. Log Button - Logs all event properties to console
 */
@Composable
fun EventsDemo(
    onBack: () -> Unit = {}
) {
    val scrollState = rememberScrollState()
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Rive Events Demo",
            style = MaterialTheme.typography.headlineSmall
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        Column(
            modifier = Modifier
                .weight(1f)
                .verticalScroll(scrollState)
        ) {
            // Star Rating Demo
            StarRatingDemo()
            
            HorizontalDivider(modifier = Modifier.padding(vertical = 16.dp))
            
            // URL Button Demo
            UrlButtonDemo()
            
            HorizontalDivider(modifier = Modifier.padding(vertical = 16.dp))
            
            // Log Button Demo
            LogButtonDemo()
        }
        
        Spacer(modifier = Modifier.height(8.dp))
        
        Button(
            onClick = onBack,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Back")
        }
    }
}

@Composable
private fun StarRatingDemo() {
    var starRating by remember { mutableStateOf<Number?>(null) }
    
    Text(
        text = "Star Rating Demo",
        style = MaterialTheme.typography.titleMedium
    )
    
    Spacer(modifier = Modifier.height(8.dp))
    
    RiveEventContent(
        demoFile = DemoRiveFile.RATING,
        stateMachineName = "State Machine 1",
        onEvent = { event ->
            if (event is RiveGeneralEvent) {
                println("RiveEvent: General event received, name: ${event.name}, " +
                        "delay: ${event.delay}, properties: ${event.properties}")
                val rating = event.properties["rating"]
                if (rating is Number) {
                    starRating = rating
                }
            }
        },
        modifier = Modifier
            .fillMaxWidth()
            .height(200.dp)
    )
    
    Text(
        text = "Star rating: ${starRating?.toString() ?: "none selected"}",
        style = MaterialTheme.typography.titleLarge
    )
}

@Composable
private fun UrlButtonDemo() {
    val uriHandler = LocalUriHandler.current
    
    Text(
        text = "URL Button Demo",
        style = MaterialTheme.typography.titleMedium
    )
    
    Text(
        text = "Button (URL Event): Open https://rive.app",
        style = MaterialTheme.typography.bodyMedium
    )
    
    Spacer(modifier = Modifier.height(8.dp))
    
    RiveEventContent(
        demoFile = DemoRiveFile.URL_EVENT_BUTTON,
        stateMachineName = "button",
        onEvent = { event ->
            if (event is RiveOpenURLEvent) {
                println("RiveEvent: Open URL Rive event: ${event.url}")
                try {
                    uriHandler.openUri(event.url)
                } catch (e: Exception) {
                    println("RiveEvent: Not a valid URL ${event.url}")
                }
            }
        },
        modifier = Modifier
            .fillMaxWidth()
            .height(150.dp)
    )
}

@Composable
private fun LogButtonDemo() {
    Text(
        text = "Log Button Demo",
        style = MaterialTheme.typography.titleMedium
    )
    
    Text(
        text = "Button (General Event): Console Log",
        style = MaterialTheme.typography.bodyMedium
    )
    
    Spacer(modifier = Modifier.height(8.dp))
    
    RiveEventContent(
        demoFile = DemoRiveFile.LOG_EVENT_BUTTON,
        stateMachineName = "button",
        onEvent = { event ->
            when (event) {
                is RiveOpenURLEvent -> {
                    println("RiveEvent: Open URL Rive event: ${event.url}")
                }
                is RiveGeneralEvent -> {
                    println("RiveEvent: General Rive event")
                }
            }
            println("RiveEvent: name: ${event.name}")
            println("RiveEvent: delay: ${event.delay}")
            println("RiveEvent: type: ${event.type}")
            println("RiveEvent: properties: ${event.properties}")
            println("RiveEvent: data: ${event.data}")
        },
        modifier = Modifier
            .fillMaxWidth()
            .height(150.dp)
    )
}

@OptIn(ExperimentalRiveComposeAPI::class)
@Composable
private fun RiveEventContent(
    demoFile: DemoRiveFile,
    stateMachineName: String,
    onEvent: (RiveEvent) -> Unit,
    modifier: Modifier = Modifier
) {
    var loadedBytes by remember { mutableStateOf<ByteArray?>(null) }
    
    LaunchedEffect(demoFile) {
        loadedBytes = DemoRiveFile.loadBytes(demoFile)
    }
    
    val bytes = loadedBytes
    if (bytes == null) {
        CircularProgressIndicator()
        return
    }
    
    val riveWorker = rememberRiveWorker()
    val fileResult = rememberRiveFile(
        source = RiveFileSource.Bytes(bytes),
        riveWorker = riveWorker
    )
    
    when (fileResult) {
        is Result.Loading -> CircularProgressIndicator()
        is Result.Error -> Text("Error: ${fileResult.throwable.message}")
        is Result.Success -> {
            Rive(
                file = fileResult.value,
                modifier = modifier,
                fit = Fit.Cover(),
                onEvent = onEvent
            )
        }
    }
}
```

#### 7.4 Update App.kt

```kotlin
package app.rive.mpapp

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import app.rive.mprive.getRivePlatform

/**
 * Represents the different screens in the app.
 */
enum class Screen {
    Main,
    RiveDemo,
    EventsDemo  // NEW
}

/**
 * Main application composable.
 * This is shared across all platforms.
 */
@Composable
fun App() {
    var currentScreen by remember { mutableStateOf(Screen.Main) }
    
    MaterialTheme {
        Surface(
            modifier = Modifier.fillMaxSize().safeContentPadding(),
            color = MaterialTheme.colorScheme.background
        ) {
            when (currentScreen) {
                Screen.Main -> MainScreen(
                    onNavigateToDemo = { currentScreen = Screen.RiveDemo },
                    onNavigateToEvents = { currentScreen = Screen.EventsDemo }
                )
                Screen.RiveDemo -> RiveDemo(
                    onBack = { currentScreen = Screen.Main }
                )
                Screen.EventsDemo -> EventsDemo(
                    onBack = { currentScreen = Screen.Main }
                )
            }
        }
    }
}

@Composable
fun MainScreen(
    onNavigateToDemo: () -> Unit = {},
    onNavigateToEvents: () -> Unit = {}
) {
    val platform = remember { getRivePlatform() }
    var initialized by remember { mutableStateOf(false) }
    
    LaunchedEffect(Unit) {
        initialized = platform.initialize()
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "Rive Multiplatform App",
            style = MaterialTheme.typography.headlineMedium
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        Text(
            text = "Platform: ${platform.getPlatformName()}",
            style = MaterialTheme.typography.bodyLarge
        )
        
        Spacer(modifier = Modifier.height(8.dp))
        
        Text(
            text = "Initialized: $initialized",
            style = MaterialTheme.typography.bodyMedium,
            color = if (initialized) {
                MaterialTheme.colorScheme.primary
            } else {
                MaterialTheme.colorScheme.error
            }
        )
        
        Spacer(modifier = Modifier.height(24.dp))
        
        // Scrollable list of demo buttons (replaces About card)
        LazyColumn(
            modifier = Modifier.fillMaxWidth(0.8f),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            item {
                Button(
                    onClick = onNavigateToDemo,
                    enabled = initialized,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Open Rive Demo")
                }
            }
            
            item {
                Button(
                    onClick = onNavigateToEvents,
                    enabled = initialized,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Events Demo")
                }
            }
        }
    }
}
```

---

## Implementation Order

### Step 1: C++ Event Infrastructure (Phase 1)
1. Add command types for event retrieval
2. Create RiveEventData structure
3. Implement event extraction in command server
4. Add property extraction from CustomPropertyContainer

### Step 2: JNI/Bindings (Phase 2)
1. Add JNI functions for event count and retrieval
2. Convert C++ event data to JNI-compatible format

### Step 3: Kotlin Event Classes (Phase 3) ✅ COMPLETE
1. ✅ Create EventType enum
2. ✅ Create RiveEvent base class
3. ✅ Create RiveGeneralEvent class
4. ✅ Create RiveOpenURLEvent class
5. ✅ Create RiveEventData transfer class

### Step 4: CommandQueue Event Methods (Phase 4)
1. Add event retrieval methods to CommandQueueBridge
2. Add pollEvents() to CommandQueue

### Step 5: StateMachine pollEvents (Phase 5)
1. Add pollEvents() method to StateMachine

### Step 6: Rive Composable Update (Phase 6)
1. Add `onEvent` parameter to Rive composable signature
2. Update Android implementation with event polling in render loop
3. Update Desktop stub implementation

### Step 7: Demo App (Phase 7)
1. Copy Rive asset files
2. Update DemoRiveFiles enum
3. Create EventsDemo screen
4. Update App.kt with scrollable navigation
5. Remove About card

---

## Files Summary

### New Files (10 total)
- `mprive/src/nativeInterop/cpp/include/rive_event.hpp`
- `mprive/src/commonMain/kotlin/app/rive/mp/event/EventType.kt` ✅ Created
- `mprive/src/commonMain/kotlin/app/rive/mp/event/RiveEvent.kt` ✅ Created
- `mprive/src/commonMain/kotlin/app/rive/mp/event/RiveGeneralEvent.kt` ✅ Created
- `mprive/src/commonMain/kotlin/app/rive/mp/event/RiveOpenURLEvent.kt` ✅ Created
- `mprive/src/commonMain/kotlin/app/rive/mp/event/RiveEventData.kt` ✅ Created
- `mpapp/src/commonMain/kotlin/app/rive/mpapp/EventsDemo.kt`
- `mpapp/src/commonMain/composeResources/files/url_event_button.riv` (copy)
- `mpapp/src/commonMain/composeResources/files/log_event_button.riv` (copy)

### Modified Files (11 total)
- `mprive/src/nativeInterop/cpp/include/command_types.hpp`
- `mprive/src/nativeInterop/cpp/include/command_server.hpp`
- `mprive/src/nativeInterop/cpp/src/command_server/command_server_state_machine.cpp`
- `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue_state_machine.cpp`
- `mprive/src/commonMain/kotlin/app/rive/mp/core/CommandQueueBridge.kt`
- `mprive/src/commonMain/kotlin/app/rive/mp/CommandQueue.kt`
- `mprive/src/commonMain/kotlin/app/rive/mp/StateMachine.kt`
- `mprive/src/commonMain/kotlin/app/rive/mp/compose/Rive.kt`
- `mprive/src/androidMain/kotlin/app/rive/mp/compose/Rive.android.kt`
- `mprive/src/desktopMain/kotlin/app/rive/mp/compose/Rive.desktop.kt`
- `mpapp/src/commonMain/kotlin/app/rive/mpapp/App.kt`
- `mpapp/src/commonMain/kotlin/app/rive/mpapp/DemoRiveFiles.kt`

---

## Testing Strategy

1. **Unit Tests**: Test event class creation and property access
2. **Integration Tests**: Test event flow from C++ through to Kotlin
3. **UI Tests**: Verify Events demo screens work correctly
4. **Cross-Platform**: Verify events work on both Android and Desktop (when Desktop support is complete)

---

## Notes

- Event classes are **pure Kotlin data classes** (no JNI pointers) for multiplatform compatibility
- Uses Compose-idiomatic **callback pattern** instead of listener pattern
- Events are polled after each `advance()` call in the render loop
- The `onEvent` callback is invoked on the composition thread