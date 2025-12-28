package app.rive.sprites


import androidx.annotation.ColorInt
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import app.rive.Artboard
import app.rive.RenderBuffer
import app.rive.RiveFile
import app.rive.RiveLog
import app.rive.StateMachine
import app.rive.ViewModelInstance
import app.rive.ViewModelSource
import app.rive.core.ArtboardHandle
import app.rive.core.CommandQueue
import app.rive.core.StateMachineHandle
import app.rive.runtime.kotlin.core.Alignment
import app.rive.runtime.kotlin.core.Fit
import app.rive.sprites.matrix.TransMatrix
import app.rive.sprites.matrix.TransMatrixData
import app.rive.sprites.props.SpriteControlProp
import kotlinx.coroutines.flow.Flow
import kotlin.time.Duration

private const val RIVE_SPRITE_TAG = "Rive/Sprite"

/**
 * A Rive sprite that can be positioned, scaled, and rotated within a [RiveSpriteScene].
 *
 * Each sprite wraps a Rive [Artboard] and [StateMachine], providing transform properties
 * for game engine integration. Sprites are rendered in a single GPU pass by the scene,
 * sorted by [zIndex].
 *
 * ## Transform Properties
 *
 * - [position]: The location of the sprite's origin in DrawScope coordinates
 * - [size]: The display size in DrawScope units (artboard is scaled to fit)
 * - [scale]: Additional scaling applied after size (supports non-uniform scaling)
 * - [rotation]: Rotation in degrees around the origin
 * - [origin]: The pivot point for transformations (default: center)
 * - [zIndex]: Rendering order (higher values render on top)
 *
 * ## ViewModel Property Interaction
 *
 * Sprites can have a [ViewModelInstance] for data binding. Use the property methods
 * to interact with ViewModel properties:
 *
 * - [setNumber], [setString], [setBoolean], [setEnum], [setColor]: Set property values
 * - [fireTrigger]: Fire a trigger property
 * - [getNumberFlow], [getStringFlow], etc.: Observe property changes reactively
 *
 * ## Tagging for Batch Operations
 *
 * Sprites can be tagged for grouping and batch operations via [RiveSpriteScene]:
 * - [tags]: Set of tags assigned to this sprite
 * - [hasTag], [hasAnyTag], [hasAllTags]: Check tag membership
 *
 * ## Example
 *
 * ```kotlin
 * // Define property descriptors
 * object EnemyProps {
 *     val Health = SpriteControlProp.Number("health")
 *     val Attack = SpriteControlProp.Trigger("attack")
 * }
 *
 * // Create sprite with auto-binding
 * val enemy = scene.createSprite(
 *     file = enemyFile,
 *     stateMachineName = "combat",
 *     viewModelConfig = SpriteViewModelConfig.AutoBind,
 *     tags = setOf(SpriteTag("enemy"))
 * )
 *
 * // Set properties
 * enemy.setNumber(EnemyProps.Health, 100f)
 * enemy.fireTrigger(EnemyProps.Attack)
 *
 * // Or use path strings
 * enemy.setNumber("health", 50f)
 * enemy.fireTrigger("attack")
 * ```
 *
 * @param artboard The Rive artboard instance for this sprite.
 * @param stateMachine The Rive state machine instance for this sprite.
 * @param commandQueue The command queue for GPU operations.
 * @param viewModelInstance Optional ViewModelInstance for data binding.
 * @param initialTags Initial tags for grouping this sprite.
 * @param ownsViewModelInstance Whether this sprite owns (and should close) the VMI.
 */
class RiveSprite internal constructor(
    val artboard: Artboard,
    val stateMachine: StateMachine,
    internal val commandQueue: CommandQueue,
    private val viewModelInstance: ViewModelInstance? = null,
    initialTags: Set<SpriteTag> = emptySet(),
    private val ownsViewModelInstance: Boolean = true,
) : IRiveSprite(initialTags) {


    // region Render Buffer Cache

    /**
     * Cached render buffer for this sprite.
     * The buffer is reused across frames to avoid expensive GPU resource allocation.
     * Only recreated when the sprite's effective size changes.
     */
    private var cachedRenderBuffer: RenderBuffer? = null
    private var cachedBufferWidth: Int = 0
    private var cachedBufferHeight: Int = 0

    /**
     * Get or create a render buffer for this sprite.
     *
     * The buffer is cached and reused across frames. It is only recreated when
     * the sprite's effective size changes.
     *
     * @return A render buffer sized to this sprite's effective size.
     */
    internal fun getOrCreateRenderBuffer(): RenderBuffer {
        val currentSize = effectiveSize
        val requiredWidth = currentSize.width.toInt().coerceAtLeast(1)
        val requiredHeight = currentSize.height.toInt().coerceAtLeast(1)

        val existingBuffer = cachedRenderBuffer
        if (existingBuffer != null &&
            cachedBufferWidth == requiredWidth &&
            cachedBufferHeight == requiredHeight
        ) {
            return existingBuffer
        }

        // Size changed or no buffer exists - create a new one
        existingBuffer?.close()

        RiveLog.d(RIVE_SPRITE_TAG) {
            "Creating render buffer ${requiredWidth}x$requiredHeight " +
                "(was ${cachedBufferWidth}x$cachedBufferHeight)"
        }

        val newBuffer = RenderBuffer(requiredWidth, requiredHeight, commandQueue)
        cachedRenderBuffer = newBuffer
        cachedBufferWidth = requiredWidth
        cachedBufferHeight = requiredHeight
        return newBuffer
    }

    // endregion

    // region Computed Properties

    /**
     * The artboard handle for native operations.
     */
    val artboardHandle: ArtboardHandle
        get() = artboard.artboardHandle

    /**
     * The state machine handle for native operations.
     */
    val stateMachineHandle: StateMachineHandle
        get() = stateMachine.stateMachineHandle

    /**
     * Whether this sprite has a ViewModelInstance for data binding.
     */
    val hasViewModel: Boolean
        get() = viewModelInstance != null

    // endregion

    // region ViewModel Property Methods

    /**
     * Set a number property value by path.
     *
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * @param propertyPath The path to the property (slash-delimited for nested) or input name
     * @param value The value to set
     */
    override fun setNumber(propertyPath: String, value: Float) {
        if (viewModelInstance != null) {
            viewModelInstance.setNumber(propertyPath, value)
        } else {
            // Fallback to legacy state machine input mechanism
            commandQueue.setStateMachineNumberInput(stateMachineHandle, propertyPath, value)
        }
    }

    /**
     * Get a Flow of number property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Float values, or null if no ViewModelInstance
     */
    override fun getNumberFlow(prop: SpriteControlProp.Number): Flow<Float>? =
        viewModelInstance?.getNumberFlow(prop.name)

    /**
     * Get a Flow of number property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Float values, or null if no ViewModelInstance
     */
    override fun getNumberFlow(propertyPath: String): Flow<Float>? =
        viewModelInstance?.getNumberFlow(propertyPath)

    /**
     * Set a string property value by path.
     *
     * Note: String properties are only available via ViewModelInstance.
     * Legacy Rive files without ViewModels do not support string inputs.
     *
     * @param propertyPath The path to the property
     * @param value The value to set
     */
    override fun setString(propertyPath: String, value: String) {
        viewModelInstance?.setString(propertyPath, value)
            ?: logPropertyWarning("setString", propertyPath)
    }

    /**
     * Get a Flow of string property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    override fun getStringFlow(prop: SpriteControlProp.Text): Flow<String>? =
        viewModelInstance?.getStringFlow(prop.name)

    /**
     * Get a Flow of string property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    override fun getStringFlow(propertyPath: String): Flow<String>? =
        viewModelInstance?.getStringFlow(propertyPath)

    /**
     * Set a boolean property value by path.
     *
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * @param propertyPath The path to the property (slash-delimited for nested) or input name
     * @param value The value to set
     */
    override fun setBoolean(propertyPath: String, value: Boolean) {
        if (viewModelInstance != null) {
            viewModelInstance.setBoolean(propertyPath, value)
        } else {
            // Fallback to legacy state machine input mechanism
            commandQueue.setStateMachineBooleanInput(stateMachineHandle, propertyPath, value)
        }
    }

    /**
     * Get a Flow of boolean property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Boolean values, or null if no ViewModelInstance
     */
    override fun getBooleanFlow(prop: SpriteControlProp.Toggle): Flow<Boolean>? =
        viewModelInstance?.getBooleanFlow(prop.name)

    /**
     * Get a Flow of boolean property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Boolean values, or null if no ViewModelInstance
     */
    override fun getBooleanFlow(propertyPath: String): Flow<Boolean>? =
        viewModelInstance?.getBooleanFlow(propertyPath)


    /**
     * Set an enum property value by path.
     *
     * @param propertyPath The path to the property
     * @param value The enum value as a string
     */
    override fun setEnum(propertyPath: String, value: String) {
        viewModelInstance?.setEnum(propertyPath, value)
            ?: logPropertyWarning("setEnum", propertyPath)
    }

    /**
     * Get a Flow of enum property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    override fun getEnumFlow(prop: SpriteControlProp.Choice): Flow<String>? =
        viewModelInstance?.getEnumFlow(prop.name)

    /**
     * Get a Flow of enum property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of String values, or null if no ViewModelInstance
     */
    override fun getEnumFlow(propertyPath: String): Flow<String>? =
        viewModelInstance?.getEnumFlow(propertyPath)

    /**
     * Set a color property value by path.
     *
     * @param propertyPath The path to the property
     * @param value The color as ARGB integer
     */
    override fun setColor(propertyPath: String, @ColorInt value: Int) {
        viewModelInstance?.setColor(propertyPath, value)
            ?: logPropertyWarning("setColor", propertyPath)
    }

    /**
     * Get a Flow of color property values for reactive observation.
     *
     * @param prop The property descriptor
     * @return A Flow of Int (ARGB) values, or null if no ViewModelInstance
     */
    override fun getColorFlow(prop: SpriteControlProp.Color): Flow<Int>? =
        viewModelInstance?.getColorFlow(prop.name)

    /**
     * Get a Flow of color property values by path.
     *
     * @param propertyPath The path to the property
     * @return A Flow of Int (ARGB) values, or null if no ViewModelInstance
     */
    override fun getColorFlow(propertyPath: String): Flow<Int>? =
        viewModelInstance?.getColorFlow(propertyPath)

    /**
     * Fire a trigger property by path.
     *
     * This method automatically uses the appropriate mechanism:
     * - If the sprite has a ViewModelInstance, it uses ViewModel property binding
     * - If no ViewModelInstance, it falls back to legacy SMI (state machine input) mechanism
     *
     * @param propertyPath The path to the trigger property or input name
     */
    override fun fireTrigger(propertyPath: String) {
        if (viewModelInstance != null) {
            viewModelInstance.fireTrigger(propertyPath)
        } else {
            // Fallback to legacy state machine input mechanism
            commandQueue.fireStateMachineTrigger(stateMachineHandle, propertyPath)
        }
    }

    /**
     * Get a Flow of trigger events for reactive observation.
     *
     * @param prop The trigger property descriptor
     * @return A Flow of Unit values (emits when trigger fires), or null if no ViewModelInstance
     */
    override fun getTriggerFlow(prop: SpriteControlProp.Trigger): Flow<Unit>? =
        viewModelInstance?.getTriggerFlow(prop.name)

    /**
     * Get a Flow of trigger events by path.
     *
     * @param propertyPath The path to the trigger property
     * @return A Flow of Unit values, or null if no ViewModelInstance
     */
    override fun getTriggerFlow(propertyPath: String): Flow<Unit>? =
        viewModelInstance?.getTriggerFlow(propertyPath)

    private fun logPropertyWarning(operation: String, propertyPath: String) {
        if (logPropertyWarnings) {
            RiveLog.w(RIVE_SPRITE_TAG) {
                "$operation('$propertyPath') skipped: no ViewModelInstance"
            }
        }
    }

    // endregion

    // region Animation Control

    /**
     * Advance the state machine by the given time delta.
     *
     * This progresses all animations and state transitions. Should be called
     * once per frame with the time elapsed since the last frame.
     *
     * @param deltaTime The time to advance by.
     */
    override fun advance(deltaTime: Duration) {
        stateMachine.advance(deltaTime)
    }

    // endregion

    // region Pointer Events

    /**
     * Send a pointer down event to the state machine.
     *
     * This is used for interactive Rive animations that respond to touch/click.
     * The point should be in DrawScope coordinates (same as [position]).
     *
     * The method transforms the point from DrawScope coordinates to artboard
     * coordinates before forwarding to the state machine.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return True if the point is within the sprite bounds and the event was sent.
     */
    override fun pointerDown(point: Offset, pointerID: Int): Boolean {
        val localPoint = transformPointToLocal(point) ?: return false
        val bounds = getArtboardBounds()

        if (!bounds.contains(localPoint)) {
            return false
        }

        // Forward to state machine using artboard coordinates
        // Use FILL fit and artboard size as surface size so coords pass through unchanged
        // layoutScale is 1f because it only applies to Fit.LAYOUT; for other fit types it's always 1f
        val displaySize = effectiveSize
        commandQueue.pointerDown(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            layoutScale = 1f,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = localPoint.x,
            pointerY = localPoint.y
        )
        return true
    }

    /**
     * Send a pointer move event to the state machine.
     *
     * This is used for hover effects and drag interactions in Rive animations.
     * The point should be in DrawScope coordinates (same as [position]).
     *
     * The method transforms the point from DrawScope coordinates to artboard
     * coordinates before forwarding to the state machine.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return True if the event was sent (even if outside bounds, for proper hover exit handling).
     */
    override fun pointerMove(point: Offset, pointerID: Int): Boolean {
        val localPoint = transformPointToLocal(point) ?: return false

        // Always send move events - the state machine handles in/out transitions
        val displaySize = effectiveSize
        commandQueue.pointerMove(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            layoutScale = 1f,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = localPoint.x,
            pointerY = localPoint.y
        )
        return true
    }

    /**
     * Send a pointer up event to the state machine.
     *
     * This is used to complete click/tap interactions in Rive animations.
     * The point should be in DrawScope coordinates (same as [position]).
     *
     * The method transforms the point from DrawScope coordinates to artboard
     * coordinates before forwarding to the state machine.
     *
     * @param point The pointer position in DrawScope coordinates.
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     * @return True if the event was sent.
     */
    override fun pointerUp(point: Offset, pointerID: Int): Boolean {
        val localPoint = transformPointToLocal(point) ?: return false

        val displaySize = effectiveSize
        commandQueue.pointerUp(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            layoutScale = 1f,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = localPoint.x,
            pointerY = localPoint.y
        )
        return true
    }

    /**
     * Send a pointer exit event to the state machine.
     *
     * This is used when the pointer leaves the sprite's area. Call this when
     * a pointer that was previously over this sprite moves away.
     *
     * @param pointerID The ID of the pointer (for multi-touch), defaults to 0.
     */
    override fun pointerExit(pointerID: Int) {
        val displaySize = effectiveSize
        // Use a point outside the bounds to signal exit
        commandQueue.pointerExit(
            stateMachineHandle = stateMachineHandle,
            fit = Fit.FILL,
            alignment = Alignment.CENTER,
            layoutScale = 1f,
            surfaceWidth = displaySize.width,
            surfaceHeight = displaySize.height,
            pointerID = pointerID,
            pointerX = -1f,
            pointerY = -1f
        )
    }

    /**
     * Get the artboard bounds in local (untransformed) coordinates.
     *
     * Used internally for hit testing to determine if a point falls within
     * the sprite's artboard area.
     *
     * @return The artboard [Rect] with origin at (0, 0).
     */
    internal fun getArtboardBounds(): Rect {
        val displaySize = effectiveSize
        return Rect(0f, 0f, displaySize.width, displaySize.height)
    }

    /**
     * Check if a point in DrawScope coordinates hits this sprite.
     *
     * @param point The point to test in DrawScope coordinates.
     * @return True if the point is within the sprite's bounds.
     */
    override fun hitTest(point: Offset): Boolean {
        if (!isVisible) return false

        val localPoint = transformPointToLocal(point) ?: return false
        val bounds = getArtboardBounds()

        return bounds.contains(localPoint)
    }


    // endregion

    // region Lifecycle

    /**
     * Release the resources held by this sprite.
     *
     * This closes the cached render buffer, artboard, state machine,
     * and optionally the ViewModelInstance.
     * The sprite should not be used after calling close.
     */
    override fun close() {
        // Close cached render buffer first
        cachedRenderBuffer?.close()
        cachedRenderBuffer = null
        cachedBufferWidth = 0
        cachedBufferHeight = 0

        // Close ViewModelInstance if we own it
        if (ownsViewModelInstance) {
            viewModelInstance?.close()
        }

        stateMachine.close()
        artboard.close()
    }

    // endregion

    companion object {
        /**
         * Global setting to log property operation warnings.
         *
         * When true, warns when property operations are skipped because
         * the sprite has no ViewModelInstance. Useful during development.
         */
        var logPropertyWarnings: Boolean = false

        /**
         * Create a new [RiveSprite] from a [RiveFile].
         *
         * @param file The Rive file to create the sprite from.
         * @param artboardName The name of the artboard to use, or null for the default.
         * @param stateMachineName The name of the state machine to use, or null for the default.
         * @param viewModelConfig Configuration for ViewModelInstance creation.
         * @param tags Initial tags for grouping this sprite.
         * @return A new [RiveSprite] instance.
         */
        fun fromFile(
            file: RiveFile,
            artboardName: String? = null,
            stateMachineName: String? = null,
            viewModelConfig: SpriteViewModelConfig = SpriteViewModelConfig.None,
            tags: Set<SpriteTag> = emptySet()
        ): RiveSprite {
            val artboard = Artboard.fromFile(file, artboardName)
            val stateMachine = StateMachine.fromArtboard(artboard, stateMachineName)

            // Create ViewModelInstance based on config
            val (vmi, ownsVmi) = createViewModelInstance(file, artboard, viewModelConfig)

            // Bind VMI to state machine if available
            if (vmi != null) {
                try {
                    file.commandQueue.bindViewModelInstance(
                        stateMachine.stateMachineHandle,
                        vmi.instanceHandle
                    )
                } catch (e: Exception) {
                    RiveLog.w(RIVE_SPRITE_TAG) {
                        "Failed to bind ViewModelInstance: ${e.message}"
                    }
                }
            }

            return RiveSprite(
                artboard = artboard,
                stateMachine = stateMachine,
                commandQueue = file.commandQueue,
                viewModelInstance = vmi,
                initialTags = tags,
                ownsViewModelInstance = ownsVmi
            )
        }

        private fun createViewModelInstance(
            file: RiveFile,
            artboard: Artboard,
            config: SpriteViewModelConfig
        ): Pair<ViewModelInstance?, Boolean> {
            return when (config) {
                is SpriteViewModelConfig.None -> {
                    null to false
                }

                is SpriteViewModelConfig.AutoBind -> {
                    // Try to create default VMI for artboard
                    try {
                        val vmi = ViewModelInstance.fromFile(
                            file,
                            ViewModelSource.DefaultForArtboard(artboard).defaultInstance()
                        )
                        vmi to true
                    } catch (e: Exception) {
                        RiveLog.d(RIVE_SPRITE_TAG) {
                            "No default ViewModel for artboard: ${e.message}"
                        }
                        null to false
                    }
                }

                is SpriteViewModelConfig.Named -> {
                    val vmi = ViewModelInstance.fromFile(
                        file,
                        ViewModelSource.Named(config.viewModelName).defaultInstance()
                    )
                    vmi to true
                }

                is SpriteViewModelConfig.NamedInstance -> {
                    val vmi = ViewModelInstance.fromFile(
                        file,
                        ViewModelSource.Named(config.viewModelName)
                            .namedInstance(config.instanceName)
                    )
                    vmi to true
                }

                is SpriteViewModelConfig.External -> {
                    // External VMI - we don't own it
                    config.instance to false
                }
            }
        }
    }
}
