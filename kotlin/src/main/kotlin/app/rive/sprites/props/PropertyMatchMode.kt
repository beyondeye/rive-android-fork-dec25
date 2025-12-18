package app.rive.sprites.props

/**
 * Defines how property names are matched when resolving [SpriteControlProp] to actual
 * ViewModelInstance properties.
 *
 * Different match modes serve different purposes:
 * - [EXACT] and [EXACT_IGNORE_CASE] for production with known property names
 * - [STARTS_WITH], [ENDS_WITH], [CONTAINS] for pattern-based matching
 * - [FUZZY] for development when property names may have typos or abbreviations
 *
 * ## Example
 *
 * ```kotlin
 * // Production: exact match (fast, no ambiguity)
 * val health = SpriteControlProp.Number("health", matchMode = PropertyMatchMode.EXACT)
 *
 * // Development: fuzzy match with logging
 * val health = SpriteControlProp.Number(
 *     "hlth",  // typo or abbreviation
 *     matchMode = PropertyMatchMode.FUZZY,
 *     logMatches = true
 * )
 *
 * // Pattern matching
 * val enemyProp = SpriteControlProp.Number(
 *     "enemy_",
 *     matchMode = PropertyMatchMode.STARTS_WITH
 * )
 * ```
 */
enum class PropertyMatchMode {
    /**
     * Exact case-sensitive match only.
     * The property name must match exactly, including case.
     */
    EXACT,

    /**
     * Exact match, case-insensitive.
     * The property name must match exactly, but case is ignored.
     * This is the default mode.
     */
    EXACT_IGNORE_CASE,

    /**
     * Property name starts with the given pattern (case-insensitive).
     * Useful for matching properties with common prefixes like "enemy_health", "enemy_speed".
     */
    STARTS_WITH,

    /**
     * Property name ends with the given pattern (case-insensitive).
     * Useful for matching properties with common suffixes like "health_modifier", "speed_modifier".
     */
    ENDS_WITH,

    /**
     * Property name contains the given pattern (case-insensitive).
     * Useful for matching properties where the pattern can appear anywhere.
     */
    CONTAINS,

    /**
     * Fuzzy match using Levenshtein distance.
     * Finds the property name with the smallest edit distance to the pattern.
     * 
     * This is useful during development when:
     * - Property names might have typos
     * - You're using abbreviations
     * - You want easier binding without knowing exact names
     *
     * **Note:** This mode is slower than exact matching and should be used
     * primarily during development. Consider switching to [EXACT] or
     * [EXACT_IGNORE_CASE] for production.
     */
    FUZZY;

    companion object {
        /**
         * Default match mode for production use.
         * Uses case-insensitive exact matching for reliability and performance.
         */
        val DEFAULT = EXACT_IGNORE_CASE

        /**
         * Match mode recommended during development for easier binding.
         * Uses fuzzy matching to tolerate typos and abbreviations.
         */
        val DEVELOPMENT = FUZZY
    }
}
