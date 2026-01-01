package app.rive.mp.sprites

import kotlin.jvm.JvmInline

/**
 * A tag that can be assigned to sprites for grouping and batch operations.
 *
 * Tags allow you to categorize sprites and perform operations on groups of sprites
 * using [RiveSpriteScene] batch methods. A sprite can have multiple tags.
 *
 * ## Common Use Cases
 *
 * - **Type grouping:** "enemy", "player", "npc", "obstacle"
 * - **State grouping:** "active", "dead", "spawning"
 * - **Layer grouping:** "foreground", "background", "ui"
 * - **Team grouping:** "team_red", "team_blue"
 *
 * ## Example
 *
 * ```kotlin
 * // Define tags
 * object Tags {
 *     val ENEMY = SpriteTag("enemy")
 *     val FLYING = SpriteTag("flying")
 *     val GROUND = SpriteTag("ground")
 *     val BOSS = SpriteTag("boss")
 * }
 *
 * // Create sprites with tags
 * val flyingEnemy = scene.createSprite(
 *     file = enemyFile,
 *     tags = setOf(Tags.ENEMY, Tags.FLYING)
 * )
 *
 * val groundEnemy = scene.createSprite(
 *     file = enemyFile,
 *     tags = setOf(Tags.ENEMY, Tags.GROUND)
 * )
 *
 * val boss = scene.createSprite(
 *     file = bossFile,
 *     tags = setOf(Tags.ENEMY, Tags.BOSS)
 * )
 *
 * // Batch operations
 * scene.fireTrigger(Tags.ENEMY, attackTrigger)  // All enemies attack
 * scene.setBoolean(Tags.FLYING, isAleratedProp, true)  // Alert flying enemies
 *
 * // Query sprites by tag
 * val allEnemies = scene.getSpritesWithTag(Tags.ENEMY)
 * val flyingEnemies = scene.getSpritesWithAllTags(Tags.ENEMY, Tags.FLYING)
 * ```
 *
 * @property value The tag string value (case-sensitive)
 */
@JvmInline
value class SpriteTag(val value: String) {
    init {
        require(value.isNotBlank()) { "SpriteTag value cannot be blank" }
    }

    override fun toString(): String = "SpriteTag($value)"

    companion object {
        /**
         * Create a tag from a string.
         *
         * Convenience method for creating tags from strings.
         */
        operator fun invoke(value: String): SpriteTag = SpriteTag(value)
    }
}

/**
 * Extension function to create a [SpriteTag] from a String.
 *
 * ```kotlin
 * val enemy = "enemy".toSpriteTag()
 * ```
 */
fun String.toSpriteTag(): SpriteTag = SpriteTag(this)

/**
 * Extension function to create a Set of [SpriteTag]s from vararg Strings.
 *
 * ```kotlin
 * val tags = spriteTags("enemy", "flying", "boss")
 * ```
 */
fun spriteTags(vararg tags: String): Set<SpriteTag> =
    tags.map { SpriteTag(it) }.toSet()
