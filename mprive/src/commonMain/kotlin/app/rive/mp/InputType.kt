package app.rive.mp

/**
 * Represents the type of a state machine input.
 */
enum class InputType(val value: Int) {
    NUMBER(0),
    BOOLEAN(1),
    TRIGGER(2),
    UNKNOWN(-1);

    companion object {
        /**
         * Get an InputType from its integer value (used for JNI callbacks).
         */
        fun fromValue(value: Int): InputType = when (value) {
            0 -> NUMBER
            1 -> BOOLEAN
            2 -> TRIGGER
            else -> UNKNOWN
        }
    }
}

/**
 * Information about a state machine input.
 */
data class InputInfo(
    val name: String,
    val type: InputType
)
