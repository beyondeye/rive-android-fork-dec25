# RiveSprite ViewModel Properties Implementation Plan

## Overview

This document outlines the implementation plan for integrating ViewModel properties into RiveSprites, enabling type-safe, reactive control of Rive animations through the new ViewModel/ViewModelInstance mechanism.

---

## Goals

1. **Replace legacy stub methods** (`fire()`, `setBoolean()`, `setNumber()`) with ViewModel-based property access
2. **Type-safe property descriptors** via `SpriteControlProp` sealed class
3. **Flexible property matching** - exact, fuzzy, contains, startsWith, endsWith
4. **Auto-discovery** of ViewModel properties at runtime
5. **Sprite grouping** via tags for batch operations
6. **Scene-level batch operations** for manipulating multiple sprites

---

## File Structure

```
kotlin/src/main/kotlin/app/rive/sprites/
├── RiveSprite.kt              # Updated with ViewModel integration
├── RiveSpriteScene.kt         # Updated with batch operations
├── props/
│   ├── PropertyMatchMode.kt   # Match mode enum
│   ├── SpriteControlProp.kt   # Type-safe property descriptors
│   ├── PropertyMatcher.kt     # Matching utility with Levenshtein
│   ├── SpriteControlPropFactory.kt  # Auto-discovery factory
│   └── DiscoveredProps.kt     # Container for discovered properties
├── SpriteViewModelConfig.kt   # VMI creation configuration
└── SpriteTag.kt               # Tag for sprite grouping
```

---

## Implementation Checklist

### Phase 1: Core Classes

- [ ] **1.1 Create `PropertyMatchMode.kt`**
  - [ ] Define enum with EXACT, EXACT_IGNORE_CASE, STARTS_WITH, ENDS_WITH, CONTAINS, FUZZY
  - [ ] Add DEFAULT and DEVELOPMENT companion constants

- [ ] **1.2 Create `SpriteControlProp.kt`**
  - [ ] Define sealed class with name, type, matchMode, logMatches properties
  - [ ] Implement Number, Text, Toggle, Choice, Color, Trigger subclasses
  - [ ] Add withMatchMode() method for copying with different settings

- [ ] **1.3 Create `PropertyMatcher.kt`**
  - [ ] Implement findMatch() with all match modes
  - [ ] Implement levenshteinDistance() for fuzzy matching
  - [ ] Add logging support for matched properties

- [ ] **1.4 Create `DiscoveredProps.kt`**
  - [ ] Define data class with lists for each property type
  - [ ] Add `all` lazy property for flat list
  - [ ] Add operator get() for name lookup
  - [ ] Add debug() for printing discovered properties

- [ ] **1.5 Create `SpriteControlPropFactory.kt`**
  - [ ] Implement fromRiveFile() suspend function
  - [ ] Map PropertyDataType to SpriteControlProp subclasses

- [ ] **1.6 Create `SpriteViewModelConfig.kt`**
  - [ ] Define sealed class with None, AutoBind, Named, NamedInstance, External options

- [ ] **1.7 Create `SpriteTag.kt`**
  - [ ] Define inline value class
  - [ ] Add NONE companion constant

### Phase 2: RiveSprite Integration

- [ ] **2.1 Add ViewModelInstance support**
  - [ ] Add viewModelInstance parameter to constructor
  - [ ] Add hasViewModel computed property
  - [ ] Add property resolution cache

- [ ] **2.2 Add tag support**
  - [ ] Add tags property (Set<SpriteTag>)
  - [ ] Add hasTag(), hasAnyTag(), hasAllTags() methods

- [ ] **2.3 Add property convenience methods**
  - [ ] setNumber(prop/path, value)
  - [ ] setString(prop/path, value)
  - [ ] setBoolean(prop/path, value)
  - [ ] setEnum(prop/path, value)
  - [ ] setColor(prop/path, value)
  - [ ] fireTrigger(prop/path)
  - [ ] getNumberFlow(), getStringFlow(), etc.

- [ ] **2.4 Update fromFile() factory**
  - [ ] Add viewModelConfig parameter
  - [ ] Add tags parameter
  - [ ] Implement VMI creation based on config
  - [ ] Implement auto-binding when configured

- [ ] **2.5 Cleanup**
  - [ ] Remove legacy stub methods (fire, setBoolean, setNumber)
  - [ ] Add logPropertyWarnings companion property

### Phase 3: RiveSpriteScene Integration

- [ ] **3.1 Add tag-based selection**
  - [ ] getSpritesWithTag()
  - [ ] getSpritesWithAnyTag()
  - [ ] getSpritesWithAllTags()

- [ ] **3.2 Add batch property operations**
  - [ ] fireTrigger(tag, prop/path)
  - [ ] setNumber(tag, prop/path, value)
  - [ ] setBoolean(tag, prop/path, value)
  - [ ] setString(tag, prop/path, value)
  - [ ] setEnum(tag, prop/path, value)
  - [ ] setColor(tag, prop/path, value)

- [ ] **3.3 Update createSprite()**
  - [ ] Add viewModelConfig parameter
  - [ ] Add tags parameter

### Phase 4: Testing & Documentation

- [ ] **4.1 Update existing tests if affected**
- [ ] **4.2 Add KDoc to all public APIs**
- [ ] **4.3 Update rive-sprite-scene-implementation.md with new APIs**

---

## API Reference

### PropertyMatchMode

```kotlin
enum class PropertyMatchMode {
    EXACT,              // Case-sensitive exact match
    EXACT_IGNORE_CASE,  // Case-insensitive exact match (DEFAULT)
    STARTS_WITH,        // Property starts with pattern
    ENDS_WITH,          // Property ends with pattern
    CONTAINS,           // Property contains pattern
    FUZZY               // Levenshtein distance matching (DEVELOPMENT)
}
```

### SpriteControlProp

```kotlin
sealed class SpriteControlProp<T>(
    val name: String,
    val type: PropertyDataType,
    val matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
    val logMatches: Boolean = false
) {
    class Number(name, matchMode, logMatches) : SpriteControlProp<Float>
    class Text(name, matchMode, logMatches) : SpriteControlProp<String>
    class Toggle(name, matchMode, logMatches) : SpriteControlProp<Boolean>
    class Choice(name, matchMode, logMatches) : SpriteControlProp<String>
    class Color(name, matchMode, logMatches) : SpriteControlProp<Int>
    class Trigger(name, matchMode, logMatches) : SpriteControlProp<Unit>
}
```

### SpriteViewModelConfig

```kotlin
sealed class SpriteViewModelConfig {
    object None : SpriteViewModelConfig()
    object AutoBind : SpriteViewModelConfig()
    data class Named(val viewModelName: String) : SpriteViewModelConfig()
    data class NamedInstance(val viewModelName: String, val instanceName: String)
    data class External(val instance: ViewModelInstance) : SpriteViewModelConfig()
}
```

### Usage Examples

```kotlin
// Define props
object EnemyProps {
    val Health = SpriteControlProp.Number("health")
    val Attack = SpriteControlProp.Trigger("attack")
}

// Create sprite with auto-binding
val enemy = scene.createSprite(
    file = enemyFile,
    viewModelConfig = SpriteViewModelConfig.AutoBind,
    tags = setOf(SpriteTag("enemy"), SpriteTag("ground"))
)

// Control individual sprite
enemy.setNumber(EnemyProps.Health, 100f)
enemy.fireTrigger(EnemyProps.Attack)

// Batch operations
scene.fireTrigger(SpriteTag("enemy"), EnemyProps.Attack)

// Auto-discover properties
val props = SpriteControlPropFactory.fromRiveFile(file, "EnemyVM")
println(props.debug())
```

---

## Implementation Progress

| Phase | Status | Notes |
|-------|--------|-------|
| Phase 1: Core Classes | ✅ Complete | Created PropertyMatchMode, SpriteControlProp, PropertyMatcher, DiscoveredProps, SpriteControlPropFactory, SpriteViewModelConfig, SpriteTag |
| Phase 2: RiveSprite Integration | ✅ Complete | Added viewModelInstance, tags, property methods (setNumber, setString, setBoolean, setEnum, setColor, fireTrigger), reactive flows, updated fromFile() |
| Phase 3: RiveSpriteScene Integration | ✅ Complete | Added tag-based selection (getSpritesWithTag, getSpritesWithAnyTag, getSpritesWithAllTags), batch property operations, updated createSprite() |
| Phase 4: Testing & Documentation | ⏳ Ready | KDoc added to all public APIs; user testing and README updates pending |

---

## Notes

- All new files go in `kotlin/src/main/kotlin/app/rive/sprites/` or `kotlin/src/main/kotlin/app/rive/sprites/props/`
- Uses existing `ViewModelInstance` from `app.rive.ViewModelInstance`
- Property flows from ViewModelInstance are cold flows that subscribe on collect
- Auto-binding requires the RiveFile to have a ViewModel configured for the artboard
