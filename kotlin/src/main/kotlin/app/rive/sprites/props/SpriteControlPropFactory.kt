package app.rive.sprites.props

import app.rive.RiveFile
import app.rive.runtime.kotlin.core.ViewModel.PropertyDataType

/**
 * Factory for auto-discovering and generating [SpriteControlProp] instances from a ViewModel.
 *
 * This factory queries a [RiveFile] for its ViewModel properties and creates
 * type-safe [SpriteControlProp] instances for each discovered property.
 *
 * ## Use Cases
 *
 * - **Development:** Quickly discover what properties are available in a ViewModel
 * - **Debugging:** Print all available properties with [DiscoveredProps.debug]
 * - **Dynamic binding:** Access properties by name at runtime when property names
 *   aren't known at compile time
 *
 * ## Example
 *
 * ```kotlin
 * // Auto-discover all properties
 * val props = SpriteControlPropFactory.fromRiveFile(file, "EnemyViewModel")
 * println(props.debug())
 *
 * // Use discovered properties
 * props.getNumber("health")?.let { sprite.setNumber(it, 100f) }
 * props.getTrigger("attack")?.let { sprite.fireTrigger(it) }
 *
 * // Or iterate over all
 * props.triggers.forEach { trigger ->
 *     println("Available trigger: ${trigger.name}")
 * }
 * ```
 *
 * @see DiscoveredProps
 * @see SpriteControlProp
 */
object SpriteControlPropFactory {

    /**
     * Discover and generate all [SpriteControlProp] instances from a RiveFile's ViewModel.
     *
     * This method queries the RiveFile for the named ViewModel's properties and
     * creates type-safe property descriptors for each supported property type.
     *
     * **Supported property types:**
     * - NUMBER → [SpriteControlProp.Number]
     * - STRING → [SpriteControlProp.Text]
     * - BOOLEAN → [SpriteControlProp.Toggle]
     * - ENUM → [SpriteControlProp.Choice]
     * - COLOR → [SpriteControlProp.Color]
     * - TRIGGER → [SpriteControlProp.Trigger]
     *
     * **Unsupported property types** (not included in results):
     * - LIST, VIEW_MODEL, ARTBOARD, INTEGER, SYMBOL_LIST_INDEX, ASSET_IMAGE
     *
     * @param file The RiveFile containing the ViewModel
     * @param viewModelName The name of the ViewModel to query
     * @param matchMode Optional match mode to apply to all discovered props
     * @param logMatches Whether discovered props should log matches
     * @return A [DiscoveredProps] containing all discovered properties organized by type
     */
    suspend fun fromRiveFile(
        file: RiveFile,
        viewModelName: String,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ): DiscoveredProps {
        val properties = file.getViewModelProperties(viewModelName)

        val numbers = mutableListOf<SpriteControlProp.Number>()
        val texts = mutableListOf<SpriteControlProp.Text>()
        val toggles = mutableListOf<SpriteControlProp.Toggle>()
        val choices = mutableListOf<SpriteControlProp.Choice>()
        val colors = mutableListOf<SpriteControlProp.Color>()
        val triggers = mutableListOf<SpriteControlProp.Trigger>()

        for (prop in properties) {
            when (prop.type) {
                PropertyDataType.NUMBER -> {
                    numbers.add(SpriteControlProp.Number(prop.name, matchMode, logMatches))
                }
                PropertyDataType.STRING -> {
                    texts.add(SpriteControlProp.Text(prop.name, matchMode, logMatches))
                }
                PropertyDataType.BOOLEAN -> {
                    toggles.add(SpriteControlProp.Toggle(prop.name, matchMode, logMatches))
                }
                PropertyDataType.ENUM -> {
                    choices.add(SpriteControlProp.Choice(prop.name, matchMode, logMatches))
                }
                PropertyDataType.COLOR -> {
                    colors.add(SpriteControlProp.Color(prop.name, matchMode, logMatches))
                }
                PropertyDataType.TRIGGER -> {
                    triggers.add(SpriteControlProp.Trigger(prop.name, matchMode, logMatches))
                }
                // Unsupported types - skip
                else -> { /* LIST, VIEW_MODEL, ARTBOARD, etc. */ }
            }
        }

        return DiscoveredProps(
            numbers = numbers,
            texts = texts,
            toggles = toggles,
            choices = choices,
            colors = colors,
            triggers = triggers
        )
    }

    /**
     * Create a [SpriteControlProp] from a property definition.
     *
     * @param name The property name
     * @param type The property data type
     * @param matchMode Match mode for the property
     * @param logMatches Whether to log matches
     * @return The appropriate [SpriteControlProp] subclass, or null for unsupported types
     */
    fun createProp(
        name: String,
        type: PropertyDataType,
        matchMode: PropertyMatchMode = PropertyMatchMode.DEFAULT,
        logMatches: Boolean = false
    ): SpriteControlProp<*>? = when (type) {
        PropertyDataType.NUMBER -> SpriteControlProp.Number(name, matchMode, logMatches)
        PropertyDataType.STRING -> SpriteControlProp.Text(name, matchMode, logMatches)
        PropertyDataType.BOOLEAN -> SpriteControlProp.Toggle(name, matchMode, logMatches)
        PropertyDataType.ENUM -> SpriteControlProp.Choice(name, matchMode, logMatches)
        PropertyDataType.COLOR -> SpriteControlProp.Color(name, matchMode, logMatches)
        PropertyDataType.TRIGGER -> SpriteControlProp.Trigger(name, matchMode, logMatches)
        else -> null // Unsupported types
    }
}
