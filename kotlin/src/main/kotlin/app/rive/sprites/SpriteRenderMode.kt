package app.rive.sprites


/**
 * Defines how sprites in a [RiveSpriteScene] are rendered to the screen.
 *
 * The rendering mode affects performance characteristics and is a trade-off between
 * flexibility and efficiency:
 *
 * - [PER_SPRITE]: More flexible, easier to debug, but less efficient for many sprites
 * - [BATCH]: More efficient for many sprites, but requires native batch rendering support
 *
 * ## Usage
 *
 * ```kotlin
 * // Use batch rendering for better performance
 * drawRiveSprites(scene, renderMode = SpriteRenderMode.BATCH)
 *
 * // Use per-sprite rendering for debugging or fallback
 * drawRiveSprites(scene, renderMode = SpriteRenderMode.PER_SPRITE)
 * ```
 *
 * @see RiveSpriteScene
 */
enum class SpriteRenderMode {
    /**
     * Render each sprite individually to its own buffer, then composite onto a shared bitmap.
     *
     * This is the original rendering approach that:
     * - Creates a separate [app.rive.RenderBuffer] for each sprite
     * - Renders each sprite's artboard to its buffer
     * - Composites all sprite bitmaps onto a shared Canvas using Android's 2D graphics
     *
     * ## Pros
     * - Simpler debugging (can inspect individual sprite renders)
     * - More resilient (one sprite failure doesn't affect others)
     * - Works without native batch rendering support
     *
     * ## Cons
     * - Higher memory usage (one GPU surface per sprite)
     * - More GPU context switches
     * - Slower for many sprites (linear overhead per sprite)
     *
     * ## Best For
     * - Debugging rendering issues
     * - Small number of sprites (< 20)
     * - When batch rendering has issues
     */
    PER_SPRITE,

    /**
     * Render all sprites in a single GPU batch operation.
     *
     * This approach uses [app.rive.core.CommandQueue.drawMultiple] to:
     * - Create a single shared GPU surface for the entire scene
     * - Submit all sprite transforms in one native call
     * - Render all sprites in a single GPU pass
     *
     * ## Pros
     * - Much better performance for many sprites
     * - Lower memory usage (one shared surface)
     * - Fewer GPU context switches
     * - Better cache utilization
     *
     * ## Cons
     * - Requires native batch rendering support (Phase 3 implementation)
     * - One sprite error may affect the entire render
     * - Harder to debug individual sprites
     *
     * ## Best For
     * - Production use with many sprites (20+)
     * - Game scenarios requiring 60fps with 100+ sprites
     * - Performance-critical rendering
     */
    BATCH;

    companion object {
        /**
         * The default rendering mode.
         *
         * Currently defaults to [PER_SPRITE] for maximum compatibility.
         * Once batch rendering is thoroughly tested, this may change to [BATCH].
         */
        val DEFAULT = PER_SPRITE
    }
}
