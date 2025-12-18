package app.rive.sprites.props

/**
 * Container for discovered ViewModel properties, organized by type.
 *
 * This class is returned by [SpriteControlPropFactory.fromRiveFile] and provides
 * easy access to auto-discovered properties from a ViewModel.
 *
 * ## Example
 *
 * ```kotlin
 * val props = SpriteControlPropFactory.fromRiveFile(file, "EnemyViewModel")
 * println(props.debug())
 * // Output:
 * // Discovered ViewModel Properties:
 * //   Numbers: [health, speed, damage]
 * //   Toggles: [isAlerted, isDead]
 * //   Triggers: [attack, die, spawn]
 *
 * // Access by type
 * val healthProp = props.numbers.first { it.name == "health" }
 * sprite.setNumber(healthProp, 100f)
 *
 * // Access by name
 * props["attack"]?.let { prop ->
 *     if (prop is SpriteControlProp.Trigger) {
 *         sprite.fireTrigger(prop)
 *     }
 * }
 * ```
 *
 * @property numbers List of number (Float) properties
 * @property texts List of string properties
 * @property toggles List of boolean properties
 * @property choices List of enum properties (String values)
 * @property colors List of color properties (ARGB Int)
 * @property triggers List of trigger properties
 */
data class DiscoveredProps(
    val numbers: List<SpriteControlProp.Number>,
    val texts: List<SpriteControlProp.Text>,
    val toggles: List<SpriteControlProp.Toggle>,
    val choices: List<SpriteControlProp.Choice>,
    val colors: List<SpriteControlProp.Color>,
    val triggers: List<SpriteControlProp.Trigger>
) {
    /**
     * All properties as a flat list.
     *
     * Use this when you need to iterate over all discovered properties
     * regardless of type.
     */
    val all: List<SpriteControlProp<*>> by lazy {
        numbers + texts + toggles + choices + colors + triggers
    }

    /**
     * Total number of discovered properties.
     */
    val size: Int
        get() = numbers.size + texts.size + toggles.size + choices.size + colors.size + triggers.size

    /**
     * Whether any properties were discovered.
     */
    val isEmpty: Boolean
        get() = size == 0

    /**
     * Whether properties were discovered.
     */
    val isNotEmpty: Boolean
        get() = size > 0

    /**
     * Get a property by name.
     *
     * @param name The property name to find
     * @return The property descriptor if found, null otherwise
     */
    operator fun get(name: String): SpriteControlProp<*>? =
        all.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Check if a property with the given name exists.
     *
     * @param name The property name to check
     * @return True if the property exists
     */
    operator fun contains(name: String): Boolean =
        all.any { it.name.equals(name, ignoreCase = true) }

    /**
     * Get a number property by name.
     *
     * @param name The property name
     * @return The number property if found, null if not found or wrong type
     */
    fun getNumber(name: String): SpriteControlProp.Number? =
        numbers.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Get a text property by name.
     *
     * @param name The property name
     * @return The text property if found, null if not found or wrong type
     */
    fun getText(name: String): SpriteControlProp.Text? =
        texts.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Get a toggle property by name.
     *
     * @param name The property name
     * @return The toggle property if found, null if not found or wrong type
     */
    fun getToggle(name: String): SpriteControlProp.Toggle? =
        toggles.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Get a choice (enum) property by name.
     *
     * @param name The property name
     * @return The choice property if found, null if not found or wrong type
     */
    fun getChoice(name: String): SpriteControlProp.Choice? =
        choices.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Get a color property by name.
     *
     * @param name The property name
     * @return The color property if found, null if not found or wrong type
     */
    fun getColor(name: String): SpriteControlProp.Color? =
        colors.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Get a trigger property by name.
     *
     * @param name The property name
     * @return The trigger property if found, null if not found or wrong type
     */
    fun getTrigger(name: String): SpriteControlProp.Trigger? =
        triggers.find { it.name.equals(name, ignoreCase = true) }

    /**
     * Generate a debug string listing all discovered properties.
     *
     * Useful during development to see what properties are available.
     *
     * @return A formatted string showing all properties by type
     */
    fun debug(): String = buildString {
        appendLine("Discovered ViewModel Properties ($size total):")
        if (numbers.isNotEmpty()) {
            appendLine("  Numbers (${numbers.size}): ${numbers.map { it.name }}")
        }
        if (texts.isNotEmpty()) {
            appendLine("  Texts (${texts.size}): ${texts.map { it.name }}")
        }
        if (toggles.isNotEmpty()) {
            appendLine("  Toggles (${toggles.size}): ${toggles.map { it.name }}")
        }
        if (choices.isNotEmpty()) {
            appendLine("  Choices (${choices.size}): ${choices.map { it.name }}")
        }
        if (colors.isNotEmpty()) {
            appendLine("  Colors (${colors.size}): ${colors.map { it.name }}")
        }
        if (triggers.isNotEmpty()) {
            appendLine("  Triggers (${triggers.size}): ${triggers.map { it.name }}")
        }
        if (isEmpty) {
            appendLine("  (no properties found)")
        }
    }

    override fun toString(): String =
        "DiscoveredProps(numbers=${numbers.size}, texts=${texts.size}, " +
            "toggles=${toggles.size}, choices=${choices.size}, " +
            "colors=${colors.size}, triggers=${triggers.size})"

    companion object {
        /**
         * An empty instance with no discovered properties.
         */
        val EMPTY = DiscoveredProps(
            numbers = emptyList(),
            texts = emptyList(),
            toggles = emptyList(),
            choices = emptyList(),
            colors = emptyList(),
            triggers = emptyList()
        )
    }
}
