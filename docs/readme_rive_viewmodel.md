# Rive ViewModel

Rive ViewModels enable data binding between your Android application and Rive animations. They allow you to control animation properties programmatically and react to changes made within the animation.

## Overview

The ViewModel system consists of two main classes:

- **`ViewModel`** - A description/schema of a ViewModel defined in the Rive file. Used to inspect properties and create instances.
- **`ViewModelInstance`** - An instantiated set of properties that can be bound to an artboard or state machine to control the animation.

## Basic Usage

### 1. Get a ViewModel from a Rive File

```kotlin
// Get ViewModel by name
val viewModel = file.getViewModelByName("My ViewModel")

// Or get the default ViewModel for an artboard
val viewModel = file.defaultViewModelForArtboard(artboard)

// Iterate all ViewModels
for (i in 0 until file.viewModelCount) {
    val vm = file.getViewModelByIndex(i)
}
```

### 2. Create a ViewModelInstance

```kotlin
// Create with default values from the Rive editor
val instance = viewModel.createDefaultInstance()

// Create a blank instance (all values initialized to defaults)
val instance = viewModel.createBlankInstance()

// Create from a named instance defined in the editor
val instance = viewModel.createInstanceFromName("My Instance")

// Create from an index
val instance = viewModel.createInstanceFromIndex(0)
```

### 3. Bind to an Artboard or State Machine

```kotlin
// Bind to artboard
artboard.viewModelInstance = instance

// Bind to state machine (for state transitions)
stateMachine.viewModelInstance = instance
```

### 4. Access and Modify Properties

```kotlin
// Get properties by type
val numberProp = instance.getNumberProperty("Score")
val stringProp = instance.getStringProperty("Username")
val boolProp = instance.getBooleanProperty("IsActive")
val colorProp = instance.getColorProperty("BackgroundColor")
val enumProp = instance.getEnumProperty("Status")
val triggerProp = instance.getTriggerProperty("OnClick")

// Read values
val score: Float = numberProp.value
val username: String = stringProp.value

// Set values
numberProp.value = 100f
stringProp.value = "Player1"
boolProp.value = true
colorProp.value = 0xFFFF0000.toInt()  // ARGB format
enumProp.value = "Active"

// Fire triggers
triggerProp.trigger()
```

### 5. Nested Properties

Access nested ViewModel properties using slash-delimited paths:

```kotlin
// Access nested instance
val nestedInstance = instance.getInstanceProperty("Settings")
val volume = nestedInstance.getNumberProperty("Volume")

// Or use path syntax
val volume = instance.getNumberProperty("Settings/Volume")
```

## Property Types

| Type | Class | Value Type | Notes |
|------|-------|------------|-------|
| Number | `ViewModelNumberProperty` | `Float` | |
| String | `ViewModelStringProperty` | `String` | |
| Boolean | `ViewModelBooleanProperty` | `Boolean` | |
| Color | `ViewModelColorProperty` | `Int` | ARGB format (0xAARRGGBB) |
| Enum | `ViewModelEnumProperty` | `String` | Value is the enum option name |
| Trigger | `ViewModelTriggerProperty` | N/A | Use `trigger()` to fire |
| Image | `ViewModelImageProperty` | N/A | Use `set(RiveRenderImage?)` |
| List | `ViewModelListProperty` | N/A | Collection of ViewModelInstances |
| Artboard | `ViewModelArtboardProperty` | N/A | Use `set(BindableArtboard?)` |

## Observing Property Changes

Use Kotlin Flow to observe property changes:

```kotlin
val numberProperty = instance.getNumberProperty("Score")

// Collect changes
lifecycleScope.launch {
    numberProperty.valueFlow.collect { value ->
        updateUI(value)
    }
}
```

## List Properties

Manipulate list properties containing ViewModelInstances:

```kotlin
val list = instance.getListProperty("Items")

// Read
val size = list.size
val item = list.elementAt(0)  // or list[0]

// Add items
list.add(newItem)
list.add(index = 1, newItem)

// Remove items
list.remove(item)
list.removeAt(0)

// Swap items
list.swap(0, 1)
```

## Auto-Binding

Enable automatic binding when loading a Rive file:

```kotlin
riveAnimationView.setRiveResource(R.raw.my_animation, autoBind = true)

// The instance is then accessible via:
val instance = riveAnimationView.controller.activeArtboard?.viewModelInstance
```

## Jetpack Compose (Experimental)

```kotlin
@OptIn(ExperimentalRiveComposeAPI::class)
@Composable
fun MyAnimation() {
    val commandQueue = rememberCommandQueueOrNull()
    val riveFile = rememberRiveFile(RiveFileSource.RawRes(R.raw.my_animation), commandQueue)

    when (val file = riveFile) {
        is Result.Success -> {
            // Create ViewModel instance
            val vmi = rememberViewModelInstance(
                file.value,
                ViewModelSource.Named("My ViewModel").defaultInstance()
            )

            // Observe property as Compose state
            val score by vmi.getNumberFlow("Score")
                .collectAsStateWithLifecycle(0f)

            RiveUI(
                file = file.value,
                viewModelInstance = vmi
            )

            // Update property
            Button(onClick = { vmi.setNumber("Score", score + 1) }) {
                Text("Add Point")
            }
        }
        // Handle loading/error states...
    }
}
```

## Transferring Instances Between Files

Transfer a ViewModelInstance to survive file deletion:

```kotlin
// Start transfer
val transfer = instance.transfer()

// Delete original file
originalFile.release()

// Apply to new artboard/state machine
newArtboard.receiveViewModelInstance(transfer)

// Or abort transfer
transfer.dispose()
```

## Important Notes

1. **Advance Required**: Property changes don't apply until the artboard/state machine is advanced:
   ```kotlin
   instance.getNumberProperty("Score").value = 100f
   controller.advance(0f)  // Changes now visible
   ```

2. **Memory Management**: ViewModelInstances are automatically cleaned up when their parent File is released. Use `transfer()` if you need instances to survive file deletion.

3. **Thread Safety**: Property access uses `ConcurrentHashMap` internally for safe concurrent access.

4. **Compose API is Experimental**: The `@ExperimentalRiveComposeAPI` annotation is required for Compose-based APIs.
