package app.rive.mp

/**
 * Represents the data type of a ViewModel property.
 * Values mirror those in rive::DataType.
 */
enum class PropertyDataType(val value: Int) {
    NONE(0),
    STRING(1),
    NUMBER(2),
    BOOLEAN(3),
    COLOR(4),
    LIST(5),
    ENUM(6),
    TRIGGER(7),
    VIEW_MODEL(8),
    INTEGER(9),
    SYMBOL_LIST_INDEX(10),
    ASSET_IMAGE(11),
    ARTBOARD(12);

    companion object {
        private val map = entries.associateBy(PropertyDataType::value)

        /**
         * Get a PropertyDataType from its integer value (used for JNI callbacks).
         */
        fun fromValue(value: Int): PropertyDataType = map[value] ?: NONE
    }
}

/**
 * Represents an update to a ViewModel property.
 * Used with SharedFlow channels to notify subscribers of property changes.
 *
 * @param T The type of the property value.
 * @property handle The handle of the ViewModelInstance that owns the property.
 * @property propertyPath The path to the property within the ViewModelInstance.
 * @property value The new value of the property.
 */
data class PropertyUpdate<T>(
    val handle: ViewModelInstanceHandle,
    val propertyPath: String,
    val value: T
)

/**
 * Describes a property defined on a ViewModel.
 * 
 * This is a read-only description used for introspection. To get or set
 * property values, you need a ViewModelInstance handle and use the
 * corresponding CommandQueue methods.
 *
 * @property name The name of the property.
 * @property type The data type of the property.
 */
data class ViewModelProperty(
    val name: String,
    val type: PropertyDataType
) {
    companion object {
        /**
         * Create a ViewModelProperty from JNI callback parameters.
         * @param name The property name.
         * @param typeValue The integer value of the PropertyDataType.
         */
        @JvmStatic // For JNI construction
        fun fromJni(name: String, typeValue: Int): ViewModelProperty =
            ViewModelProperty(name, PropertyDataType.fromValue(typeValue))
    }
}

/**
 * Describes an enum definition in a Rive file.
 * 
 * Enums can be either system-defined or user-defined in the Rive editor.
 * This class provides the enum name and all its possible values.
 *
 * @property name The name of the enum.
 * @property values The list of possible values for this enum.
 */
data class RiveEnum(
    val name: String,
    val values: List<String>
) {
    companion object {
        /**
         * Create a RiveEnum from JNI callback parameters.
         * @param name The enum name.
         * @param values The list of enum values.
         */
        @JvmStatic // For JNI construction
        fun fromJni(name: String, values: List<String>): RiveEnum =
            RiveEnum(name, values)
    }
}