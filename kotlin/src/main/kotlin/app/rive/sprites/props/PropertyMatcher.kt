package app.rive.sprites.props

import app.rive.RiveLog
import app.rive.runtime.kotlin.core.ViewModel.PropertyDataType

private const val TAG = "Rive/PropMatcher"

/**
 * Utility for matching property names with various strategies.
 *
 * This object provides the core matching logic for [SpriteControlProp], supporting
 * different match modes from exact matching to fuzzy matching with Levenshtein distance.
 *
 * @see PropertyMatchMode
 * @see SpriteControlProp
 */
internal object PropertyMatcher {

    /**
     * Find the best matching property from available candidates.
     *
     * @param pattern The pattern to match
     * @param candidates Available property names to search
     * @param mode The match mode to use
     * @param expectedType Expected property type (for filtering)
     * @param availableProperties Map of property name to type (for type filtering)
     * @param logMatches Whether to log match results
     * @return The matched property path, or null if no match found
     */
    fun findMatch(
        pattern: String,
        candidates: List<String>,
        mode: PropertyMatchMode,
        expectedType: PropertyDataType? = null,
        availableProperties: Map<String, PropertyDataType>? = null,
        logMatches: Boolean = false
    ): String? {
        // Filter by type first if available
        val filtered = if (expectedType != null && availableProperties != null) {
            candidates.filter { availableProperties[it] == expectedType }
        } else {
            candidates
        }

        val match = when (mode) {
            PropertyMatchMode.EXACT -> findExactMatch(pattern, filtered)
            PropertyMatchMode.EXACT_IGNORE_CASE -> findExactIgnoreCaseMatch(pattern, filtered)
            PropertyMatchMode.STARTS_WITH -> findStartsWithMatch(pattern, filtered)
            PropertyMatchMode.ENDS_WITH -> findEndsWithMatch(pattern, filtered)
            PropertyMatchMode.CONTAINS -> findContainsMatch(pattern, filtered)
            PropertyMatchMode.FUZZY -> findFuzzyMatch(pattern, filtered)
        }

        if (logMatches) {
            logMatchResult(pattern, match, mode, filtered)
        }

        return match
    }

    /**
     * Find the best matching property using a [SpriteControlProp] descriptor.
     *
     * @param prop The property descriptor with match configuration
     * @param candidates Available property names to search
     * @param availableProperties Map of property name to type (for type filtering)
     * @return The matched property path, or null if no match found
     */
    fun <T> findMatch(
        prop: SpriteControlProp<T>,
        candidates: List<String>,
        availableProperties: Map<String, PropertyDataType>? = null
    ): String? = findMatch(
        pattern = prop.name,
        candidates = candidates,
        mode = prop.matchMode,
        expectedType = prop.type,
        availableProperties = availableProperties,
        logMatches = prop.logMatches
    )

    private fun findExactMatch(pattern: String, candidates: List<String>): String? =
        candidates.find { it == pattern }

    private fun findExactIgnoreCaseMatch(pattern: String, candidates: List<String>): String? =
        candidates.find { it.equals(pattern, ignoreCase = true) }

    private fun findStartsWithMatch(pattern: String, candidates: List<String>): String? =
        candidates.find { it.startsWith(pattern, ignoreCase = true) }

    private fun findEndsWithMatch(pattern: String, candidates: List<String>): String? =
        candidates.find { it.endsWith(pattern, ignoreCase = true) }

    private fun findContainsMatch(pattern: String, candidates: List<String>): String? =
        candidates.find { it.contains(pattern, ignoreCase = true) }

    /**
     * Find the best fuzzy match using Levenshtein distance.
     *
     * Returns the candidate with the smallest edit distance to the pattern,
     * but only if the distance is within an acceptable threshold
     * (50% of the pattern length, minimum 3).
     */
    private fun findFuzzyMatch(pattern: String, candidates: List<String>): String? {
        if (candidates.isEmpty()) return null

        val patternLower = pattern.lowercase()
        val scored = candidates.map { candidate ->
            candidate to levenshteinDistance(patternLower, candidate.lowercase())
        }

        // Return best match if distance is reasonable (< 50% of pattern length, min 3)
        val threshold = (pattern.length * 0.5).toInt().coerceAtLeast(3)
        return scored
            .filter { it.second <= threshold }
            .minByOrNull { it.second }
            ?.first
    }

    /**
     * Calculate the Levenshtein distance (edit distance) between two strings.
     *
     * The Levenshtein distance is the minimum number of single-character edits
     * (insertions, deletions, or substitutions) required to change one string
     * into the other.
     *
     * @param s1 First string
     * @param s2 Second string
     * @return The edit distance between the strings
     */
    internal fun levenshteinDistance(s1: String, s2: String): Int {
        if (s1.isEmpty()) return s2.length
        if (s2.isEmpty()) return s1.length

        // Optimize by using only two rows instead of full matrix
        var previousRow = IntArray(s2.length + 1) { it }
        var currentRow = IntArray(s2.length + 1)

        for (i in 1..s1.length) {
            currentRow[0] = i

            for (j in 1..s2.length) {
                val cost = if (s1[i - 1] == s2[j - 1]) 0 else 1
                currentRow[j] = minOf(
                    previousRow[j] + 1,      // deletion
                    currentRow[j - 1] + 1,   // insertion
                    previousRow[j - 1] + cost // substitution
                )
            }

            // Swap rows
            val temp = previousRow
            previousRow = currentRow
            currentRow = temp
        }

        return previousRow[s2.length]
    }

    private fun logMatchResult(
        pattern: String,
        match: String?,
        mode: PropertyMatchMode,
        candidates: List<String>
    ) {
        if (match != null) {
            if (pattern != match) {
                // Pattern didn't match exactly - log what was matched
                RiveLog.d(TAG) {
                    "Property match: '$pattern' -> '$match' (mode: $mode)"
                }
            } else {
                RiveLog.v(TAG) {
                    "Property exact match: '$pattern' (mode: $mode)"
                }
            }
        } else {
            RiveLog.w(TAG) {
                "No property match found for '$pattern' (mode: $mode, " +
                    "candidates: ${candidates.take(10)}${if (candidates.size > 10) "..." else ""})"
            }
        }
    }
}
