package app.rive.mp.event

/**
 * Target window/context for opening a URL from a [RiveOpenURLEvent].
 *
 * These values correspond to HTML link target attributes and control
 * where the URL should be opened.
 *
 * @property value The numeric value as defined in the Rive runtime.
 * @property targetName The string representation (e.g., "_blank").
 */
enum class OpenUrlTarget(val value: Int, val targetName: String) {
    /**
     * Open in a new window or tab.
     */
    Blank(0, "_blank"),

    /**
     * Open in the parent frame.
     */
    Parent(1, "_parent"),

    /**
     * Open in the same frame (replace current content).
     */
    Self(2, "_self"),

    /**
     * Open in the full body of the window (topmost frame).
     */
    Top(3, "_top");

    companion object {
        private val valueMap = entries.associateBy(OpenUrlTarget::value)
        private val nameMap = entries.associateBy(OpenUrlTarget::targetName)

        /**
         * Returns the [OpenUrlTarget] for the given numeric value.
         * Returns [Blank] if the value is not recognized.
         */
        fun fromValue(value: Int): OpenUrlTarget = valueMap[value] ?: Blank

        /**
         * Returns the [OpenUrlTarget] for the given target name string.
         * Returns [Blank] if the name is not recognized or empty.
         *
         * @param name The target name (e.g., "_blank", "_self").
         */
        fun fromName(name: String): OpenUrlTarget = nameMap[name] ?: Blank
    }
}
