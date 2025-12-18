package app.rive.sprites.props

import app.rive.runtime.kotlin.core.ViewModel.PropertyDataType

/**
 * A type-safe descriptor for a ViewModel property that can be used to interact with a sprite.
 *
 * `SpriteControlProp` provides compile-time type safety when accessing ViewModel properties
 * on [RiveSprite][app.rive.sprites.RiveSprite] instances. Instead of using raw strings,
 * you define property descriptors that encode the property name, type, and optional
 * matching configuration.
 *
 * ## Property Types
 *
 * - [Number] - Float values (health, speed, damage, etc.)
 * - [Text] - String values (names, labels, messages, etc.)
 * - [Toggle] - Boolean values (isAlive, isVisible, etc.)
 * - [Choice] - Enum values represented as strings
 * - [Color] - ARGB color values as Int
 * - [Trigger] - One-shot events (attack, jump, die, etc.)
 *
 * ## Match Modes
 *
 * By default, properties are matched case-insensitively. You can configure different
 * match modes for development (fuzzy) or pattern matching (startsWith, contains, etc.).
 *
 * ## Example
 *
 * ```kotlin
 * // Define reusable property descriptors
 * object EnemyProps {
 *     val Health = SpriteControlProp.Number("health")
 *     val IsAlerted = SpriteControlProp.Toggle("isAlerted")
 *     val Attack = SpriteControlProp.Trigger("attack")
 *     val State = SpriteControlProp.Choice("enemyState")
 * }
 *
 * // Use with sprites
 * sprite.setNumber(EnemyProps.Health, 100f)
 * sprite.setBoolean(EnemyProps.IsAlerted, true)
 * sprite.fireTrigger(EnemyProps.Attack)
 *
 * // Development mode with fuzzy matching
 * val devHealth = SpriteControlProp.Number(
 *     name = "hlth",  // typo
 *     matchMode = PropertyMatchMode.FUZZY,
 *     logMatches = true
 * )
 * ```
 *
 * @param T The Kotlin type of the property value
 * @property name The property name or pattern to match
 * @property type The expected property type from the ViewModel
 * @property matchMode How to match the property name (default: EXACT_IGNORE_CASE)
 * @property logMatches If true, logs when properties are matched (useful for debugging)
 */
sealed class SpriteControlProp<T>(
    val name: String,
    val type: PropertyDataType,
    val matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
    val logMatches: Boolean = false
) {
    /**
     * A number property (Float).
     *
     * Use for continuous values like health, speed, damage, direction, etc.
     *
     * @param name The property name or pattern
     * @param matchMode How to match the property name
     * @param logMatches Whether to log match results
     */
    class Number(
        name: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ) : SpriteControlProp<Float>(name, PropertyDataType.NUMBER, matchMode, logMatches) {
        override fun withMatchMode(
            matchMode: PropertyMatchMode,
            logMatches: Boolean
        ): Number = Number(name, matchMode, logMatches)
    }

    /**
     * A string property.
     *
     * Use for text values like names, labels, messages, etc.
     *
     * @param name The property name or pattern
     * @param matchMode How to match the property name
     * @param logMatches Whether to log match results
     */
    class Text(
        name: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ) : SpriteControlProp<String>(name, PropertyDataType.STRING, matchMode, logMatches) {
        override fun withMatchMode(
            matchMode: PropertyMatchMode,
            logMatches: Boolean
        ): Text = Text(name, matchMode, logMatches)
    }

    /**
     * A boolean property.
     *
     * Use for on/off states like isAlive, isVisible, isRunning, etc.
     *
     * @param name The property name or pattern
     * @param matchMode How to match the property name
     * @param logMatches Whether to log match results
     */
    class Toggle(
        name: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ) : SpriteControlProp<Boolean>(name, PropertyDataType.BOOLEAN, matchMode, logMatches) {
        override fun withMatchMode(
            matchMode: PropertyMatchMode,
            logMatches: Boolean
        ): Toggle = Toggle(name, matchMode, logMatches)
    }

    /**
     * An enum property (represented as String).
     *
     * Use for state values with discrete options like "idle", "walking", "running".
     *
     * @param name The property name or pattern
     * @param matchMode How to match the property name
     * @param logMatches Whether to log match results
     */
    class Choice(
        name: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ) : SpriteControlProp<String>(name, PropertyDataType.ENUM, matchMode, logMatches) {
        override fun withMatchMode(
            matchMode: PropertyMatchMode,
            logMatches: Boolean
        ): Choice = Choice(name, matchMode, logMatches)
    }

    /**
     * A color property (ARGB Int).
     *
     * Colors are represented as integers in 0xAARRGGBB format.
     *
     * @param name The property name or pattern
     * @param matchMode How to match the property name
     * @param logMatches Whether to log match results
     */
    class Color(
        name: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ) : SpriteControlProp<Int>(name, PropertyDataType.COLOR, matchMode, logMatches) {
        override fun withMatchMode(
            matchMode: PropertyMatchMode,
            logMatches: Boolean
        ): Color = Color(name, matchMode, logMatches)
    }

    /**
     * A trigger property (fires events).
     *
     * Triggers are one-shot inputs that cause state transitions without
     * maintaining a value. Use for events like attack, jump, die, spawn.
     *
     * @param name The property name or pattern
     * @param matchMode How to match the property name
     * @param logMatches Whether to log match results
     */
    class Trigger(
        name: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ) : SpriteControlProp<Unit>(name, PropertyDataType.TRIGGER, matchMode, logMatches) {
        override fun withMatchMode(
            matchMode: PropertyMatchMode,
            logMatches: Boolean
        ): Trigger = Trigger(name, matchMode, logMatches)
    }

    /**
     * Create a copy of this property descriptor with different match settings.
     *
     * Useful for temporarily changing match behavior:
     * ```kotlin
     * val devHealth = EnemyProps.Health.withMatchMode(
     *     matchMode = PropertyMatchMode.FUZZY,
     *     logMatches = true
     * )
     * ```
     *
     * @param matchMode The new match mode
     * @param logMatches Whether to log match results
     * @return A new property descriptor with the specified settings
     */
    abstract fun withMatchMode(
        matchMode: PropertyMatchMode = this.matchMode,
        logMatches: Boolean = this.logMatches
    ): SpriteControlProp<T>

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is SpriteControlProp<*>) return false
        return name == other.name && type == other.type
    }

    override fun hashCode(): Int {
        var result = name.hashCode()
        result = 31 * result + type.hashCode()
        return result
    }

    override fun toString(): String =
        "${this::class.simpleName}(name='$name', matchMode=$matchMode, logMatches=$logMatches)"
}
