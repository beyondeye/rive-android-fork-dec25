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
