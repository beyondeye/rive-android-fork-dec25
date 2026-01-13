package app.rive.mp

import app.rive.mp.core.Alignment
import app.rive.mp.core.CommandQueueBridge
import app.rive.mp.core.Fit
import app.rive.mp.core.Listeners
import app.rive.mp.core.SpriteDrawCommand
import app.rive.mp.core.createCommandQueueBridge
import kotlinx.atomicfu.atomic
import kotlinx.coroutines.CancellableContinuation
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.cancellation.CancellationException
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.time.Duration

/**
 * Type alias matching the upstream API.
 * [CommandQueue] is named to match the underlying core C++ class,
 * but for public API purposes, RiveWorker is a more semantic name.
 */
typealias RiveWorker = CommandQueue

/**
 * Additional alias for the interior type [CommandQueue.PropertyUpdate].
 */
typealias RivePropertyUpdate<T> = CommandQueue.PropertyUpdate<T>

const val COMMAND_QUEUE_TAG = "Rive/CQ"

/**
 * A [CommandQueue] is the worker that runs Rive in a thread. It holds all of the state,
 * including assets, Rive files, artboards, state machines, and view model instances.
 *
 * Instances of the command queue are reference counted. At initialization, the command queue has a
 * ref count of 1. Call [acquire] to increment the ref count, and [release] to decrement it. When
 * the ref count reaches 0, the command queue is disposed, and its resources are released.
 *
 * For Phase A, this is a minimal implementation focused on thread lifecycle management.
 *
 * @param renderContext The [RenderContext] to use for rendering. The CommandQueue takes ownership.
 * @throws IllegalStateException If the command queue cannot be created.
 */
class CommandQueue(
    private val renderContext: RenderContext = createDefaultRenderContext(),
    private val bridge: CommandQueueBridge = createCommandQueueBridge()
) : RefCounted {

    companion object {
        /**
         * Maximum number of concurrent subscribers that can safely use this CommandQueue.
         */
        const val MAX_CONCURRENT_SUBSCRIBERS = 32
    }

    /**
     * Represents an update to a property on a ViewModelInstance.
     * Emitted on property flows when subscribed properties change.
     *
     * @param vmiHandle Handle to the ViewModelInstance that owns the property.
     * @param propertyPath Path to the property that was updated.
     * @param value The new value of the property.
     */
    data class PropertyUpdate<T>(
        val vmiHandle: ViewModelInstanceHandle,
        val propertyPath: String,
        val value: T
    )
    
    /**
     * The native pointer to the CommandServer C++ object, held in a reference-counted pointer.
     */
    private val cppPointer = RCPointer(
        bridge.cppConstructor(renderContext.nativeObjectPointer),
        COMMAND_QUEUE_TAG,
        ::dispose
    )
    
    /**
     * Cleanup to be performed when the ref count reaches 0.
     *
     * Any pending continuations are cancelled to avoid callers hanging indefinitely.
     */
    private fun dispose(cppPointer: Long) {
        bridge.cppDelete(cppPointer)
        renderContext.close()
        
        // Cancel and clear any pending JNI continuations so callers don't hang.
        pendingContinuations.values.toList().forEach { cont ->
            cont.cancel(CancellationException("CommandQueue was released before operation could complete."))
        }
        pendingContinuations.clear()
    }
    
    // Implement the RefCounted interface by delegating to cppPointer
    override fun acquire(source: String) = cppPointer.acquire(source)
    override fun release(source: String, reason: String) = cppPointer.release(source, reason)
    override val refCount: Int
        get() = cppPointer.refCount
    override val isDisposed: Boolean
        get() = cppPointer.isDisposed
    
    /** Alias for [isDisposed] to satisfy [CheckableAutoCloseable] interface. */
    override val closed: Boolean
        get() = isDisposed
    
    /**
     * Flow that emits when a state machine has settled.
     * For Phase A, this is just a placeholder.
     */
    private val _settledFlow = MutableSharedFlow<StateMachineHandle>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    val settledFlow: SharedFlow<StateMachineHandle> = _settledFlow

    // =============================================================================
    // Property Flows (Phase D.4)
    // =============================================================================

    private val _numberPropertyFlow = MutableSharedFlow<PropertyUpdate<Float>>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    /** Hot flow that emits when any subscribed number property is updated. */
    val numberPropertyFlow: SharedFlow<PropertyUpdate<Float>> = _numberPropertyFlow

    private val _stringPropertyFlow = MutableSharedFlow<PropertyUpdate<String>>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    /** Hot flow that emits when any subscribed string property is updated. */
    val stringPropertyFlow: SharedFlow<PropertyUpdate<String>> = _stringPropertyFlow

    private val _booleanPropertyFlow = MutableSharedFlow<PropertyUpdate<Boolean>>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    /** Hot flow that emits when any subscribed boolean property is updated. */
    val booleanPropertyFlow: SharedFlow<PropertyUpdate<Boolean>> = _booleanPropertyFlow

    private val _enumPropertyFlow = MutableSharedFlow<PropertyUpdate<String>>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    /** Hot flow that emits when any subscribed enum property is updated. */
    val enumPropertyFlow: SharedFlow<PropertyUpdate<String>> = _enumPropertyFlow

    private val _colorPropertyFlow = MutableSharedFlow<PropertyUpdate<Int>>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    /** Hot flow that emits when any subscribed color property is updated. */
    val colorPropertyFlow: SharedFlow<PropertyUpdate<Int>> = _colorPropertyFlow

    private val _triggerPropertyFlow = MutableSharedFlow<PropertyUpdate<Unit>>(
        replay = 0,
        extraBufferCapacity = MAX_CONCURRENT_SUBSCRIBERS,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    /** Hot flow that emits when any subscribed trigger property is fired. */
    val triggerPropertyFlow: SharedFlow<PropertyUpdate<Unit>> = _triggerPropertyFlow

    init {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Creating command queue" }
    }
    
    /**
     * Poll messages from the CommandServer to the CommandQueue. This is the channel that all
     * callbacks and errors arrive on. Should be called every frame.
     *
     * For Phase A, this is a basic implementation.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun pollMessages() = bridge.cppPollMessages(cppPointer.pointer, this)
    
    /**
     * Create a Rive rendering surface for Rive to draw into.
     * Phase A stub - to be implemented in Phase C.
     */
    fun createRiveSurface(drawKey: DrawKey): RiveSurface =
        renderContext.createSurface(drawKey, this)
    
    /**
     * Create an off-screen image surface for rendering to a buffer.
     * Phase A stub - to be implemented in Phase C.
     */
    fun createImageSurface(width: Int, height: Int, drawKey: DrawKey): RiveSurface =
        renderContext.createImageSurface(width, height, drawKey, this)
    
    /**
     * Destroy a Rive rendering surface.
     * Phase A stub - to be implemented in Phase C.
     */
    fun destroyRiveSurface(surface: RiveSurface) {
        // For Phase A, just close directly
        // In Phase C, this will run on command server thread
        surface.close()
    }
    
    /**
     * Creates a Rive render target on the command server thread.
     * 
     * This method creates a render target that can be used with the [draw] method.
     * The render target is created on the worker thread where the OpenGL context is active.
     * 
     * **Note**: In the current implementation (Phase C.2.3), this returns a placeholder value (0).
     * The full implementation will be completed in Phase C.2.6 when the Rive renderer is integrated.
     * 
     * @param width The width of the render target in pixels.
     * @param height The height of the render target in pixels.
     * @return A native pointer to the created render target, or 0 if creation failed.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun createRiveRenderTarget(width: Int, height: Int): Long {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Creating Rive render target: ${width}x${height}" }
        return bridge.cppCreateRiveRenderTarget(cppPointer.pointer, width, height)
    }
    
    // =============================================================================
    // Async Request Infrastructure
    // =============================================================================
    
    /**
     * The map of all pending continuations, keyed by request ID. Entries are added when a suspend
     * request is made, and removed when the request is completed.
     */
    private val pendingContinuations = mutableMapOf<Long, CancellableContinuation<Any>>()
    
    /**
     * A monotonically increasing request ID used to identify JNI requests.
     */
    private val nextRequestID = atomic(0L)
    
    /**
     * A monotonically increasing counter for generating unique draw keys.
     * Draw keys are used to uniquely identify draw operations for surface management.
     */
    private val nextDrawKeyCounter = atomic(0L)
    
    // =============================================================================
    // Phase C.2.4: DrawKey Generation
    // =============================================================================
    
    /**
     * Create a unique draw key for identifying draw operations.
     * 
     * Draw keys are used to:
     * - Uniquely identify surfaces and render targets
     * - Track draw operations in the command queue
     * - Manage surface lifecycle
     *
     * Each call returns a new, unique key that is guaranteed to be different
     * from all previously returned keys.
     *
     * @return A unique [DrawKey] for identifying draw operations.
     * @throws IllegalStateException If the CommandQueue has been released.
     * 
     * @see draw Not yet implemented - use for single artboard rendering (Phase C.2.7)
     * @see drawMultiple Not yet implemented - use for batch sprite rendering (Phase E)
     * @see drawToBuffer Not yet implemented - use for offscreen rendering (Phase E)
     */
    @Throws(IllegalStateException::class)
    fun createDrawKey(): DrawKey {
        return DrawKey(nextDrawKeyCounter.incrementAndGet())
    }
    
    /**
     * Make a JNI request that returns a value of type [T], split across a callback.
     * Phase A stub - basic implementation for future use.
     */
    @Throws(CancellationException::class)
    private suspend inline fun <reified T> suspendNativeRequest(
        crossinline nativeFn: (Long) -> Unit
    ): T = suspendCancellableCoroutine { cont ->
        val requestID = nextRequestID.getAndIncrement()
        
        // Store the continuation
        @Suppress("UNCHECKED_CAST")
        pendingContinuations[requestID] = cont as CancellableContinuation<Any>
        
        cont.invokeOnCancellation {
            pendingContinuations.remove(requestID)
        }
        
        nativeFn(requestID)
    }
    
    // =============================================================================
    // Phase B: File Operations (stubs for Phase A)
    // =============================================================================
    
    /**
     * Load a Rive file into the command queue.
     * 
     * @param bytes The Rive file bytes to load.
     * @return A handle to the loaded file.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file cannot be loaded (e.g., malformed file).
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun loadFile(bytes: ByteArray): FileHandle {
        return suspendNativeRequest { requestID ->
            bridge.cppLoadFile(cppPointer.pointer, requestID, bytes)
        }
    }
    
    /**
     * Delete a file and free its resources.
     * 
     * @param fileHandle The handle of the file to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteFile(fileHandle: FileHandle) {
        // For Phase B.1, we use a requestID but don't wait for completion
        // In the future, this could be made async if needed
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppDeleteFile(cppPointer.pointer, requestID, fileHandle.handle)
    }
    
    /**
     * Get the names of all artboards in a file.
     * 
     * @param fileHandle The handle of the file to query.
     * @return A list of artboard names.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getArtboardNames(fileHandle: FileHandle): List<String> {
        return suspendNativeRequest { requestID ->
            bridge.cppGetArtboardNames(cppPointer.pointer, requestID, fileHandle.handle)
        }
    }
    
    /**
     * Get the names of all state machines in an artboard.
     * 
     * @param artboardHandle The handle of the artboard to query.
     * @return A list of state machine names.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the artboard handle is invalid or artboards not yet implemented.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getStateMachineNames(artboardHandle: ArtboardHandle): List<String> {
        return suspendNativeRequest { requestID ->
            bridge.cppGetStateMachineNames(cppPointer.pointer, requestID, artboardHandle.handle)
        }
    }
    
    /**
     * Get the names of all view models in a file.
     * 
     * @param fileHandle The handle of the file to query.
     * @return A list of view model names.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getViewModelNames(fileHandle: FileHandle): List<String> {
        return suspendNativeRequest { requestID ->
            bridge.cppGetViewModelNames(cppPointer.pointer, requestID, fileHandle.handle)
        }
    }
    
    /**
     * Create the default artboard from a file.
     * 
     * This is a synchronous operation that returns the artboard handle directly.
     * 
     * @param fileHandle The handle of the file to create artboard from.
     * @return A handle to the created artboard.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the file handle is invalid or artboard creation fails.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createDefaultArtboard(fileHandle: FileHandle): ArtboardHandle {
        val handle = bridge.cppCreateDefaultArtboard(cppPointer.pointer, 0L, fileHandle.handle)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create default artboard")
        }
        return ArtboardHandle(handle)
    }
    
    /**
     * Create an artboard by name from a file.
     * 
     * This is a synchronous operation that returns the artboard handle directly.
     * 
     * @param fileHandle The handle of the file to create artboard from.
     * @param name The name of the artboard to create.
     * @return A handle to the created artboard.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the file handle is invalid or artboard not found.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createArtboardByName(fileHandle: FileHandle, name: String): ArtboardHandle {
        val handle = bridge.cppCreateArtboardByName(cppPointer.pointer, 0L, fileHandle.handle, name)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create artboard '$name'")
        }
        return ArtboardHandle(handle)
    }
    
    /**
     * Delete an artboard and free its resources.
     * 
     * @param artboardHandle The handle of the artboard to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteArtboard(artboardHandle: ArtboardHandle) {
        // Fire and forget - don't wait for completion
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppDeleteArtboard(cppPointer.pointer, requestID, artboardHandle.handle)
    }

    // =============================================================================
    // Phase E.3: Artboard Resizing (for Fit.Layout)
    // =============================================================================

    /**
     * Resize an artboard to match the given dimensions.
     * 
     * This is required for [Fit.LAYOUT] mode where the artboard needs to match
     * the surface dimensions for responsive layout behavior.
     * 
     * This is a fire-and-forget operation that enqueues a resize command on the
     * render thread. The resize will take effect on the next advance/draw cycle.
     *
     * @param artboardHandle The handle of the artboard to resize.
     * @param width The new width in pixels.
     * @param height The new height in pixels.
     * @param scaleFactor Scale factor for high DPI displays. Defaults to 1.0.
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see resetArtboardSize To restore the original artboard dimensions.
     * @see Fit.LAYOUT The fit mode that requires artboard resizing.
     */
    @Throws(IllegalStateException::class)
    fun resizeArtboard(
        artboardHandle: ArtboardHandle,
        width: Int,
        height: Int,
        scaleFactor: Float = 1.0f
    ) {
        bridge.cppResizeArtboard(cppPointer.pointer, artboardHandle.handle, width, height, scaleFactor)
    }

    /**
     * Reset an artboard to its original dimensions.
     * 
     * This undoes any previous [resizeArtboard] calls and restores the artboard
     * to the dimensions defined in the Rive file.
     * 
     * This is a fire-and-forget operation that enqueues a reset command on the
     * render thread. The reset will take effect on the next advance/draw cycle.
     *
     * @param artboardHandle The handle of the artboard to reset.
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see resizeArtboard To resize the artboard to specific dimensions.
     */
    @Throws(IllegalStateException::class)
    fun resetArtboardSize(artboardHandle: ArtboardHandle) {
        bridge.cppResetArtboardSize(cppPointer.pointer, artboardHandle.handle)
    }

    // =============================================================================
    // Phase C.2.3: Render Target Operations
    // =============================================================================

    /**
     * Create a render target for offscreen rendering.
     *
     * A render target is a GPU resource used for rendering Rive content.
     * It must be created on the render thread where the GL context is active.
     *
     * @param width The width of the render target in pixels.
     * @param height The height of the render target in pixels.
     * @param sampleCount MSAA sample count (0 = no MSAA, typical values: 4, 8, 16).
     * @return A handle to the created render target.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the dimensions are invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createRenderTarget(width: Int, height: Int, sampleCount: Int = 0): Long {
        return suspendNativeRequest { requestID ->
            bridge.cppCreateRenderTarget(cppPointer.pointer, requestID, width, height, sampleCount)
        }
    }

    /**
     * Delete a render target and free its resources.
     *
     * @param renderTargetHandle The handle of the render target to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteRenderTarget(renderTargetHandle: Long) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppDeleteRenderTarget(cppPointer.pointer, requestID, renderTargetHandle)
    }

    // =============================================================================
    // Phase C.2.7: Draw Operations
    // =============================================================================

    /**
     * Draw the artboard with its current state machine state to the given surface.
     *
     * This is a fire-and-forget operation that enqueues a draw command on the
     * render thread. The actual rendering happens asynchronously.
     *
     * @param artboardHandle Handle to the artboard to draw.
     * @param smHandle Handle to the state machine (for animation state). Pass 0 for static artboards.
     * @param surface The surface to render to.
     * @param fit How to fit the artboard into the surface bounds. Defaults to CONTAIN.
     * @param alignment How to align the artboard within the surface. Defaults to CENTER.
     * @param clearColor Background clear color in 0xAARRGGBB format. Defaults to opaque black.
     * @param scaleFactor Scale factor for high DPI displays. Defaults to 1.0.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see Fit For available fit modes.
     * @see Alignment For available alignment positions.
     */
    @Throws(IllegalStateException::class)
    fun draw(
        artboardHandle: ArtboardHandle,
        smHandle: StateMachineHandle,
        surface: RiveSurface,
        fit: Fit = Fit.CONTAIN,
        alignment: Alignment = Alignment.CENTER,
        clearColor: Int = 0xFF000000.toInt(),
        scaleFactor: Float = 1.0f
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppDraw(
            cppPointer.pointer,
            requestID,
            artboardHandle.handle,
            smHandle.handle,
            surface.surfaceNativePointer,
            surface.renderTargetPointer.pointer,
            surface.drawKey.handle,
            surface.width,
            surface.height,
            fit.ordinal,
            alignment.ordinal,
            clearColor,
            scaleFactor
        )
    }

    // =============================================================================
    // Phase 0.4: Batch Sprite Rendering (RiveSpriteScene support)
    // =============================================================================

    /**
     * Draw multiple sprites in a single batch operation.
     *
     * This is an efficient way to render many Rive sprites in a single draw call.
     * All sprites share the same viewport and clear color, but each can have its
     * own artboard, state machine, transform, and dimensions.
     *
     * This is a fire-and-forget operation that enqueues a draw command on the
     * render thread. The actual rendering happens asynchronously.
     *
     * @param surface The surface to render to.
     * @param commands List of sprite draw commands specifying what to draw and how.
     * @param clearColor Background clear color in 0xAARRGGBB format. Defaults to transparent.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If commands is empty.
     *
     * @see SpriteDrawCommand For the structure of each draw command.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun drawMultiple(
        surface: RiveSurface,
        commands: List<SpriteDrawCommand>,
        clearColor: Int = 0x00000000
    ) {
        require(commands.isNotEmpty()) { "Commands list cannot be empty" }
        
        val count = commands.size
        val artboardHandles = LongArray(count)
        val stateMachineHandles = LongArray(count)
        val transforms = FloatArray(count * 6)  // 6 elements per transform
        val artboardWidths = FloatArray(count)
        val artboardHeights = FloatArray(count)
        
        commands.forEachIndexed { index, cmd ->
            artboardHandles[index] = cmd.artboardHandle.handle
            stateMachineHandles[index] = cmd.stateMachineHandle.handle
            System.arraycopy(cmd.transform, 0, transforms, index * 6, 6)
            artboardWidths[index] = cmd.artboardWidth
            artboardHeights[index] = cmd.artboardHeight
        }
        
        bridge.cppDrawMultiple(
            cppPointer.pointer,
            renderContext.nativeObjectPointer,
            surface.surfaceNativePointer,
            surface.drawKey.handle,
            surface.renderTargetPointer.pointer,
            surface.width,
            surface.height,
            clearColor,
            artboardHandles,
            stateMachineHandles,
            transforms,
            artboardWidths,
            artboardHeights,
            count
        )
    }

    /**
     * Draw multiple sprites in a single batch operation and read the result into a buffer.
     *
     * This is similar to [drawMultiple] but also copies the rendered pixels back into
     * the provided byte buffer. This is useful for offscreen rendering or when you need
     * to process the rendered image.
     *
     * This is a synchronous operation that blocks until rendering is complete and the
     * pixel data has been copied to the buffer.
     *
     * @param surface The surface to render to.
     * @param commands List of sprite draw commands specifying what to draw and how.
     * @param clearColor Background clear color in 0xAARRGGBB format. Defaults to transparent.
     * @param buffer ByteArray to receive the rendered pixels. Must be sized correctly for
     *               the surface dimensions (width * height * 4 bytes for RGBA).
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If commands is empty or buffer size is incorrect.
     *
     * @see SpriteDrawCommand For the structure of each draw command.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun drawMultipleToBuffer(
        surface: RiveSurface,
        commands: List<SpriteDrawCommand>,
        clearColor: Int = 0x00000000,
        buffer: ByteArray
    ) {
        require(commands.isNotEmpty()) { "Commands list cannot be empty" }
        val expectedSize = surface.width * surface.height * 4  // RGBA
        require(buffer.size >= expectedSize) {
            "Buffer size ${buffer.size} is too small. Expected at least $expectedSize bytes for ${surface.width}x${surface.height} RGBA."
        }
        
        val count = commands.size
        val artboardHandles = LongArray(count)
        val stateMachineHandles = LongArray(count)
        val transforms = FloatArray(count * 6)  // 6 elements per transform
        val artboardWidths = FloatArray(count)
        val artboardHeights = FloatArray(count)
        
        commands.forEachIndexed { index, cmd ->
            artboardHandles[index] = cmd.artboardHandle.handle
            stateMachineHandles[index] = cmd.stateMachineHandle.handle
            System.arraycopy(cmd.transform, 0, transforms, index * 6, 6)
            artboardWidths[index] = cmd.artboardWidth
            artboardHeights[index] = cmd.artboardHeight
        }
        
        bridge.cppDrawMultipleToBuffer(
            cppPointer.pointer,
            renderContext.nativeObjectPointer,
            surface.surfaceNativePointer,
            surface.drawKey.handle,
            surface.renderTargetPointer.pointer,
            surface.width,
            surface.height,
            clearColor,
            artboardHandles,
            stateMachineHandles,
            transforms,
            artboardWidths,
            artboardHeights,
            count,
            buffer
        )
    }

    // =============================================================================
    // Phase C.5: Pointer Events
    // =============================================================================

    /**
     * Send a pointer move event to a state machine.
     *
     * This is used for hover effects and drag operations. The coordinates should be
     * in surface/view coordinates (pixels from top-left of the rendering surface).
     * The native layer will transform these to artboard coordinates using the fit
     * and alignment settings.
     *
     * This is a fire-and-forget operation that enqueues the event on the render thread.
     *
     * @param smHandle Handle to the state machine to receive the event.
     * @param surface The surface being rendered to (for coordinate transformation).
     * @param x The x coordinate in surface pixels.
     * @param y The y coordinate in surface pixels.
     * @param fit How the artboard is fit into the surface. Defaults to CONTAIN.
     * @param alignment How the artboard is aligned within the surface. Defaults to CENTER.
     * @param scaleFactor Scale factor for high DPI displays. Defaults to 1.0.
     * @param pointerID Identifier for multi-touch support. Defaults to 0.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see pointerDown For press/click start events.
     * @see pointerUp For press/click end events.
     * @see pointerExit For when pointer leaves the surface.
     */
    @Throws(IllegalStateException::class)
    fun pointerMove(
        smHandle: StateMachineHandle,
        surface: RiveSurface,
        x: Float,
        y: Float,
        fit: Fit = Fit.CONTAIN,
        alignment: Alignment = Alignment.CENTER,
        scaleFactor: Float = 1.0f,
        pointerID: Int = 0
    ) {
        bridge.cppPointerMove(
            cppPointer.pointer,
            smHandle.handle,
            fit.ordinal.toByte(),
            alignment.ordinal.toByte(),
            scaleFactor,
            surface.width.toFloat(),
            surface.height.toFloat(),
            pointerID,
            x,
            y
        )
    }

    /**
     * Send a pointer down (press/click start) event to a state machine.
     *
     * This is used for button clicks and touch start events. The coordinates should be
     * in surface/view coordinates (pixels from top-left of the rendering surface).
     * The native layer will transform these to artboard coordinates using the fit
     * and alignment settings.
     *
     * This is a fire-and-forget operation that enqueues the event on the render thread.
     *
     * @param smHandle Handle to the state machine to receive the event.
     * @param surface The surface being rendered to (for coordinate transformation).
     * @param x The x coordinate in surface pixels.
     * @param y The y coordinate in surface pixels.
     * @param fit How the artboard is fit into the surface. Defaults to CONTAIN.
     * @param alignment How the artboard is aligned within the surface. Defaults to CENTER.
     * @param scaleFactor Scale factor for high DPI displays. Defaults to 1.0.
     * @param pointerID Identifier for multi-touch support. Defaults to 0.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see pointerUp For the corresponding release event.
     * @see pointerMove For hover/drag events.
     */
    @Throws(IllegalStateException::class)
    fun pointerDown(
        smHandle: StateMachineHandle,
        surface: RiveSurface,
        x: Float,
        y: Float,
        fit: Fit = Fit.CONTAIN,
        alignment: Alignment = Alignment.CENTER,
        scaleFactor: Float = 1.0f,
        pointerID: Int = 0
    ) {
        bridge.cppPointerDown(
            cppPointer.pointer,
            smHandle.handle,
            fit.ordinal.toByte(),
            alignment.ordinal.toByte(),
            scaleFactor,
            surface.width.toFloat(),
            surface.height.toFloat(),
            pointerID,
            x,
            y
        )
    }

    /**
     * Send a pointer up (press/click end) event to a state machine.
     *
     * This is used for button clicks and touch end events. The coordinates should be
     * in surface/view coordinates (pixels from top-left of the rendering surface).
     * The native layer will transform these to artboard coordinates using the fit
     * and alignment settings.
     *
     * This is a fire-and-forget operation that enqueues the event on the render thread.
     *
     * @param smHandle Handle to the state machine to receive the event.
     * @param surface The surface being rendered to (for coordinate transformation).
     * @param x The x coordinate in surface pixels.
     * @param y The y coordinate in surface pixels.
     * @param fit How the artboard is fit into the surface. Defaults to CONTAIN.
     * @param alignment How the artboard is aligned within the surface. Defaults to CENTER.
     * @param scaleFactor Scale factor for high DPI displays. Defaults to 1.0.
     * @param pointerID Identifier for multi-touch support. Defaults to 0.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see pointerDown For the corresponding press event.
     * @see pointerMove For hover/drag events.
     */
    @Throws(IllegalStateException::class)
    fun pointerUp(
        smHandle: StateMachineHandle,
        surface: RiveSurface,
        x: Float,
        y: Float,
        fit: Fit = Fit.CONTAIN,
        alignment: Alignment = Alignment.CENTER,
        scaleFactor: Float = 1.0f,
        pointerID: Int = 0
    ) {
        bridge.cppPointerUp(
            cppPointer.pointer,
            smHandle.handle,
            fit.ordinal.toByte(),
            alignment.ordinal.toByte(),
            scaleFactor,
            surface.width.toFloat(),
            surface.height.toFloat(),
            pointerID,
            x,
            y
        )
    }

    /**
     * Send a pointer exit event to a state machine.
     *
     * This is used when the pointer leaves the rendering surface entirely.
     * It allows the state machine to cancel any hover states.
     *
     * This is a fire-and-forget operation that enqueues the event on the render thread.
     *
     * @param smHandle Handle to the state machine to receive the event.
     * @param surface The surface being rendered to (for coordinate transformation context).
     * @param fit How the artboard is fit into the surface. Defaults to CONTAIN.
     * @param alignment How the artboard is aligned within the surface. Defaults to CENTER.
     * @param scaleFactor Scale factor for high DPI displays. Defaults to 1.0.
     * @param pointerID Identifier for multi-touch support. Defaults to 0.
     *
     * @throws IllegalStateException If the CommandQueue has been released.
     *
     * @see pointerMove For re-entry events.
     */
    @Throws(IllegalStateException::class)
    fun pointerExit(
        smHandle: StateMachineHandle,
        surface: RiveSurface,
        fit: Fit = Fit.CONTAIN,
        alignment: Alignment = Alignment.CENTER,
        scaleFactor: Float = 1.0f,
        pointerID: Int = 0
    ) {
        // For exit, we pass 0,0 as coordinates since they're not meaningful
        bridge.cppPointerExit(
            cppPointer.pointer,
            smHandle.handle,
            fit.ordinal.toByte(),
            alignment.ordinal.toByte(),
            scaleFactor,
            surface.width.toFloat(),
            surface.height.toFloat(),
            pointerID,
            0f,
            0f
        )
    }

    // =============================================================================
    // Phase C: State Machine Operations
    // =============================================================================
    
    // NOTE: All external fun declarations have been moved to CommandQueueBridge interface.
    // Methods now use bridge.cppXxx() pattern for platform abstraction.


    /**
     * Create the default state machine from an artboard.
     * 
     * This is a synchronous operation that returns the state machine handle directly.
     * 
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @return A handle to the created state machine.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the artboard handle is invalid or state machine creation fails.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createDefaultStateMachine(artboardHandle: ArtboardHandle): StateMachineHandle {
        val handle = bridge.cppCreateDefaultStateMachine(cppPointer.pointer, 0L, artboardHandle.handle)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create default state machine")
        }
        return StateMachineHandle(handle)
    }
    
    /**
     * Create a state machine by name from an artboard.
     * 
     * This is a synchronous operation that returns the state machine handle directly.
     * 
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @param name The name of the state machine to create.
     * @return A handle to the created state machine.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the artboard handle is invalid or state machine not found.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createStateMachineByName(artboardHandle: ArtboardHandle, name: String): StateMachineHandle {
        val handle = bridge.cppCreateStateMachineByName(cppPointer.pointer, 0L, artboardHandle.handle, name)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create state machine '$name'")
        }
        return StateMachineHandle(handle)
    }
    
    /**
     * Advance a state machine by a time delta.
     * 
     * Note: This is a synchronous operation (fire-and-forget). It does not wait for the state machine to settle.
     * If you need to be notified when the state machine settles, subscribe to [settledFlow].
     * 
     * @param smHandle The handle of the state machine to advance.
     * @param deltaTimeSeconds The time delta in seconds.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun advanceStateMachine(smHandle: StateMachineHandle, deltaTimeSeconds: Float) {
        // Fire and forget - don't wait for completion
        val deltaTimeNs = (deltaTimeSeconds * 1_000_000_000L).toLong()
        bridge.cppAdvanceStateMachine(cppPointer.pointer, smHandle.handle, deltaTimeNs)
    }
    
    /**
     * Delete a state machine and free its resources.
     *
     * @param smHandle The handle of the state machine to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteStateMachine(smHandle: StateMachineHandle) {
        // Fire and forget - don't wait for completion
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppDeleteStateMachine(cppPointer.pointer, requestID, smHandle.handle)
    }

    // =============================================================================
    // Phase 0.3: State Machine Input Manipulation (SMI - for RiveSprite)
    // =============================================================================

    /**
     * Set a number input on a state machine (fire-and-forget).
     * This is a simplified API for RiveSprite that doesn't wait for confirmation.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the number input.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setStateMachineNumberInput(smHandle: StateMachineHandle, inputName: String, value: Float) {
        bridge.cppSetStateMachineNumberInput(cppPointer.pointer, smHandle.handle, inputName, value)
    }

    /**
     * Set a boolean input on a state machine (fire-and-forget).
     * This is a simplified API for RiveSprite that doesn't wait for confirmation.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the boolean input.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setStateMachineBooleanInput(smHandle: StateMachineHandle, inputName: String, value: Boolean) {
        bridge.cppSetStateMachineBooleanInput(cppPointer.pointer, smHandle.handle, inputName, value)
    }

    /**
     * Fire a trigger input on a state machine (fire-and-forget).
     * This is a simplified API for RiveSprite that doesn't wait for confirmation.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the trigger input.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun fireStateMachineTrigger(smHandle: StateMachineHandle, inputName: String) {
        bridge.cppFireStateMachineTrigger(cppPointer.pointer, smHandle.handle, inputName)
    }

    // =============================================================================
    // Phase C.4: State Machine Input Operations
    // =============================================================================

    /**
     * Get the number of inputs in a state machine.
     *
     * @param smHandle The handle of the state machine.
     * @return The number of inputs.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the state machine handle is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getInputCount(smHandle: StateMachineHandle): Int {
        return suspendNativeRequest { requestID ->
            bridge.cppGetInputCount(cppPointer.pointer, requestID, smHandle.handle)
        }
    }

    /**
     * Get the names of all inputs in a state machine.
     *
     * @param smHandle The handle of the state machine.
     * @return A list of input names.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the state machine handle is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getInputNames(smHandle: StateMachineHandle): List<String> {
        return suspendNativeRequest { requestID ->
            bridge.cppGetInputNames(cppPointer.pointer, requestID, smHandle.handle)
        }
    }

    /**
     * Get information about an input by index.
     *
     * @param smHandle The handle of the state machine.
     * @param inputIndex The index of the input (0-based).
     * @return The input information (name and type).
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the state machine handle or input index is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getInputInfo(smHandle: StateMachineHandle, inputIndex: Int): InputInfo {
        return suspendNativeRequest { requestID ->
            bridge.cppGetInputInfo(cppPointer.pointer, requestID, smHandle.handle, inputIndex)
        }
    }

    /**
     * Get the value of a number input.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     * @return The current value of the input.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the state machine handle or input name is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getNumberInput(smHandle: StateMachineHandle, inputName: String): Float {
        return suspendNativeRequest { requestID ->
            bridge.cppGetNumberInput(cppPointer.pointer, requestID, smHandle.handle, inputName)
        }
    }

    /**
     * Set the value of a number input.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setNumberInput(smHandle: StateMachineHandle, inputName: String, value: Float) {
        // Fire and forget - don't wait for completion
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetNumberInput(cppPointer.pointer, requestID, smHandle.handle, inputName, value)
    }

    /**
     * Get the value of a boolean input.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     * @return The current value of the input.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the state machine handle or input name is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getBooleanInput(smHandle: StateMachineHandle, inputName: String): Boolean {
        return suspendNativeRequest { requestID ->
            bridge.cppGetBooleanInput(cppPointer.pointer, requestID, smHandle.handle, inputName)
        }
    }

    /**
     * Set the value of a boolean input.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the input.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setBooleanInput(smHandle: StateMachineHandle, inputName: String, value: Boolean) {
        // Fire and forget - don't wait for completion
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetBooleanInput(cppPointer.pointer, requestID, smHandle.handle, inputName, value)
    }

    /**
     * Fire a trigger input.
     *
     * @param smHandle The handle of the state machine.
     * @param inputName The name of the trigger input.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun fireTrigger(smHandle: StateMachineHandle, inputName: String) {
        // Fire and forget - don't wait for completion
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppFireTrigger(cppPointer.pointer, requestID, smHandle.handle, inputName)
    }

    // =============================================================================
    // Phase D.1: ViewModelInstance Creation
    // =============================================================================

    /**
     * Create a blank ViewModelInstance from a named ViewModel.
     * A blank instance has all properties set to their default values.
     *
     * This is a synchronous operation that returns the VMI handle directly.
     *
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to create an instance of.
     * @return A handle to the created ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the file handle is invalid or ViewModel not found.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createBlankViewModelInstance(
        fileHandle: FileHandle,
        viewModelName: String
    ): ViewModelInstanceHandle {
        val handle = bridge.cppCreateBlankVMI(cppPointer.pointer, 0L, fileHandle.handle, viewModelName)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create blank ViewModelInstance for '$viewModelName'")
        }
        return ViewModelInstanceHandle(handle)
    }

    /**
     * Create a default ViewModelInstance from a named ViewModel.
     * A default instance uses the ViewModel's default instance values if available.
     *
     * This is a synchronous operation that returns the VMI handle directly.
     *
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to create an instance of.
     * @return A handle to the created ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the file handle is invalid or ViewModel not found.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createDefaultViewModelInstance(
        fileHandle: FileHandle,
        viewModelName: String
    ): ViewModelInstanceHandle {
        val handle = bridge.cppCreateDefaultVMI(cppPointer.pointer, 0L, fileHandle.handle, viewModelName)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create default ViewModelInstance for '$viewModelName'")
        }
        return ViewModelInstanceHandle(handle)
    }

    /**
     * Create a named ViewModelInstance from a named ViewModel.
     * Uses the values defined for the named instance in the Rive file.
     *
     * This is a synchronous operation that returns the VMI handle directly.
     *
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to create an instance of.
     * @param instanceName The name of the specific instance to create.
     * @return A handle to the created ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws IllegalArgumentException If the file handle is invalid, ViewModel not found, or instance not found.
     */
    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun createNamedViewModelInstance(
        fileHandle: FileHandle,
        viewModelName: String,
        instanceName: String
    ): ViewModelInstanceHandle {
        val handle = bridge.cppCreateNamedVMI(cppPointer.pointer, 0L, fileHandle.handle, viewModelName, instanceName)
        if (handle == 0L) {
            throw IllegalArgumentException("Failed to create named ViewModelInstance '$instanceName' for '$viewModelName'")
        }
        return ViewModelInstanceHandle(handle)
    }

    /**
     * Delete a ViewModelInstance and free its resources.
     *
     * @param vmiHandle The handle of the ViewModelInstance to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteViewModelInstance(vmiHandle: ViewModelInstanceHandle) {
        // Fire and forget - don't wait for completion
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppDeleteVMI(cppPointer.pointer, requestID, vmiHandle.handle)
    }

    // =============================================================================
    // Phase D.2: Property Operations
    // =============================================================================

    /**
     * Get a number property value from a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property (e.g., "myNumber" or "nested/property").
     * @return The current value of the property.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getNumberProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): Float {
        return suspendNativeRequest { requestID ->
            bridge.cppGetNumberProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Set a number property value on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setNumberProperty(vmiHandle: ViewModelInstanceHandle, propertyPath: String, value: Float) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetNumberProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, value)
    }

    /**
     * Get a string property value from a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @return The current value of the property.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getStringProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): String {
        return suspendNativeRequest { requestID ->
            bridge.cppGetStringProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Set a string property value on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setStringProperty(vmiHandle: ViewModelInstanceHandle, propertyPath: String, value: String) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetStringProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, value)
    }

    /**
     * Get a boolean property value from a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @return The current value of the property.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getBooleanProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): Boolean {
        return suspendNativeRequest { requestID ->
            bridge.cppGetBooleanProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Set a boolean property value on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The value to set.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setBooleanProperty(vmiHandle: ViewModelInstanceHandle, propertyPath: String, value: Boolean) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetBooleanProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, value)
    }

    // =============================================================================
    // Phase D.3: Additional Property Types (enum, color, trigger)
    // =============================================================================

    /**
     * Get an enum property value from a ViewModelInstance.
     * The value is returned as a string representing the selected enum option.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @return The current enum value as a string.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getEnumProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): String {
        return suspendNativeRequest { requestID ->
            bridge.cppGetEnumProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Set an enum property value on a ViewModelInstance.
     * The value must be a valid enum option string.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The enum value to set (as a string).
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setEnumProperty(vmiHandle: ViewModelInstanceHandle, propertyPath: String, value: String) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetEnumProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, value)
    }

    /**
     * Get a color property value from a ViewModelInstance.
     * The color is returned in 0xAARRGGBB format.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @return The current color value in 0xAARRGGBB format.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getColorProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): Int {
        return suspendNativeRequest { requestID ->
            bridge.cppGetColorProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Set a color property value on a ViewModelInstance.
     * The color should be in 0xAARRGGBB format.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The color value in 0xAARRGGBB format.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setColorProperty(vmiHandle: ViewModelInstanceHandle, propertyPath: String, value: Int) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetColorProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, value)
    }

    /**
     * Fire a trigger property on a ViewModelInstance.
     * This is a one-shot action that triggers any bound behaviors.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the trigger property.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun fireTriggerProperty(vmiHandle: ViewModelInstanceHandle, propertyPath: String) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppFireTriggerProperty(cppPointer.pointer, vmiHandle.handle, propertyPath)
    }

    // =============================================================================
    // Phase D.4: Property Subscriptions
    // =============================================================================

    /**
     * Subscribe to changes to a property on a ViewModelInstance.
     * Updates will be emitted on the flow of the corresponding type:
     * - [numberPropertyFlow] for NUMBER properties
     * - [stringPropertyFlow] for STRING properties
     * - [booleanPropertyFlow] for BOOLEAN properties
     * - [enumPropertyFlow] for ENUM properties
     * - [colorPropertyFlow] for COLOR properties
     * - [triggerPropertyFlow] for TRIGGER properties
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the property.
     * @param propertyPath The path to the property within the ViewModelInstance.
     * @param propertyType The data type of the property.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun subscribeToProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        propertyType: PropertyDataType
    ) {
        bridge.cppSubscribeToProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, propertyType.value)
    }

    /**
     * Unsubscribe from changes to a property on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the property.
     * @param propertyPath The path to the property within the ViewModelInstance.
     * @param propertyType The data type of the property.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun unsubscribeFromProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        propertyType: PropertyDataType
    ) {
        bridge.cppUnsubscribeFromProperty(cppPointer.pointer, vmiHandle.handle, propertyPath, propertyType.value)
    }

    // =============================================================================
    // Phase D.5: List Operations
    // =============================================================================

    /**
     * Get the size of a list property on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the list property.
     * @return The number of items in the list.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getListSize(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): Int {
        return suspendNativeRequest { requestID ->
            bridge.cppGetListSize(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Get an item from a list property by index.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the list property.
     * @param index The index of the item to get (0-based).
     * @return A handle to the ViewModelInstance at the specified index.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid, property not found, or index out of bounds.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getListItem(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        index: Int
    ): ViewModelInstanceHandle {
        return suspendNativeRequest { requestID ->
            bridge.cppGetListItem(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, index)
        }
    }

    /**
     * Add an item to the end of a list property.
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the list.
     * @param propertyPath The path to the list property.
     * @param itemHandle The handle of the ViewModelInstance to add.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun addListItem(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        itemHandle: ViewModelInstanceHandle
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppAddListItem(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, itemHandle.handle)
    }

    /**
     * Add an item at a specific index in a list property.
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the list.
     * @param propertyPath The path to the list property.
     * @param index The index at which to insert the item.
     * @param itemHandle The handle of the ViewModelInstance to add.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun addListItemAt(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        index: Int,
        itemHandle: ViewModelInstanceHandle
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppAddListItemAt(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, index, itemHandle.handle)
    }

    /**
     * Remove an item from a list property by its handle.
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the list.
     * @param propertyPath The path to the list property.
     * @param itemHandle The handle of the ViewModelInstance to remove.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun removeListItem(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        itemHandle: ViewModelInstanceHandle
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppRemoveListItem(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, itemHandle.handle)
    }

    /**
     * Remove an item from a list property by index.
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the list.
     * @param propertyPath The path to the list property.
     * @param index The index of the item to remove.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun removeListItemAt(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        index: Int
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppRemoveListItemAt(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, index)
    }

    /**
     * Swap two items in a list property.
     *
     * @param vmiHandle The handle of the ViewModelInstance that owns the list.
     * @param propertyPath The path to the list property.
     * @param indexA The index of the first item.
     * @param indexB The index of the second item.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun swapListItems(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        indexA: Int,
        indexB: Int
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSwapListItems(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, indexA, indexB)
    }

    // =============================================================================
    // Phase D.5: Nested VMI Operations
    // =============================================================================

    /**
     * Get a nested ViewModelInstance property from a ViewModelInstance.
     *
     * @param vmiHandle The handle of the parent ViewModelInstance.
     * @param propertyPath The path to the nested VMI property.
     * @return A handle to the nested ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the VMI handle is invalid or property not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getInstanceProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String
    ): ViewModelInstanceHandle {
        return suspendNativeRequest { requestID ->
            bridge.cppGetInstanceProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
        }
    }

    /**
     * Set a nested ViewModelInstance property on a ViewModelInstance.
     *
     * @param vmiHandle The handle of the parent ViewModelInstance.
     * @param propertyPath The path to the nested VMI property.
     * @param nestedHandle The handle of the ViewModelInstance to set as the nested value.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setInstanceProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        nestedHandle: ViewModelInstanceHandle
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetInstanceProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, nestedHandle.handle)
    }

    // =============================================================================
    // Phase D.5: Asset Property Operations
    // =============================================================================

    /**
     * Set an image property on a ViewModelInstance.
     * Pass a null imageHandle to clear the image.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the image property.
     * @param imageHandle The handle of the image to set, or null to clear.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setImageProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        imageHandle: ImageHandle?
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetImageProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, imageHandle?.handle ?: 0L)
    }

    /**
     * Set an artboard property on a ViewModelInstance.
     * Pass a null artboardHandle to clear the artboard.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the artboard property.
     * @param fileHandle The handle of the file that owns the artboard.
     * @param artboardHandle The handle of the artboard to set, or null to clear.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun setArtboardProperty(
        vmiHandle: ViewModelInstanceHandle,
        propertyPath: String,
        fileHandle: FileHandle,
        artboardHandle: ArtboardHandle?
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppSetArtboardProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, fileHandle.handle, artboardHandle?.handle ?: 0L)
    }

    // =============================================================================
    // Phase D.6: VMI Binding to State Machine
    // =============================================================================

    /**
     * Bind a ViewModelInstance to a StateMachine for data binding.
     * Once bound, changes to the VMI properties will be reflected in the state machine.
     *
     * @param smHandle The handle of the state machine to bind to.
     * @param vmiHandle The handle of the ViewModelInstance to bind.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun bindViewModelInstance(
        smHandle: StateMachineHandle,
        vmiHandle: ViewModelInstanceHandle
    ) {
        val requestID = nextRequestID.getAndIncrement()
        bridge.cppBindViewModelInstance(cppPointer.pointer, requestID, smHandle.handle, vmiHandle.handle)
    }

    /**
     * Get the default ViewModelInstance for an artboard.
     * Returns the default VMI if one exists, or null if the artboard has no default VMI.
     *
     * @param fileHandle The handle of the file that contains the artboard.
     * @param artboardHandle The handle of the artboard to get the default VMI for.
     * @return A handle to the default VMI, or null if no default VMI exists.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file or artboard handle is invalid.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun getDefaultViewModelInstance(
        fileHandle: FileHandle,
        artboardHandle: ArtboardHandle
    ): ViewModelInstanceHandle? {
        return suspendNativeRequest { requestID ->
            bridge.cppGetDefaultViewModelInstance(cppPointer.pointer, requestID, fileHandle.handle, artboardHandle.handle)
        }
    }

    // =============================================================================
    // Phase E.1: Asset Management
    // =============================================================================

    /**
     * Decode an image file from the given bytes.
     * 
     * The bytes should be for a compressed image format such as PNG or JPEG.
     * The decoded image is stored on the CommandServer and can be used with
     * [registerImage] to fulfill referenced assets in Rive files.
     *
     * @param bytes The bytes of the image file to decode.
     * @return A handle to the decoded image.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the image could not be decoded (e.g., invalid format).
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun decodeImage(bytes: ByteArray): ImageHandle {
        return suspendNativeRequest { requestID ->
            bridge.cppDecodeImage(cppPointer.pointer, requestID, bytes)
        }
    }

    /**
     * Delete an image and free its resources.
     * 
     * Counterpart to [decodeImage]. This is useful when you no longer need the
     * image and want to free up memory.
     *
     * @param imageHandle The handle of the image to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteImage(imageHandle: ImageHandle) {
        bridge.cppDeleteImage(cppPointer.pointer, imageHandle.handle)
    }

    /**
     * Register an image as an asset with the given name.
     * 
     * This allows the image to be used to fulfill a referenced asset when loading
     * a Rive file. Registrations are global to this CommandQueue, meaning that
     * the [name] will be used to fulfill any file loaded by this CommandQueue
     * that references the asset with the same name.
     *
     * The same image can be registered multiple times with different names,
     * allowing it to fulfill multiple referenced assets.
     *
     * @param name The name of the referenced asset to fulfill. Must match the name
     *             in the zip file when exporting from Rive.
     * @param imageHandle The handle of the image to register.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun registerImage(name: String, imageHandle: ImageHandle) {
        bridge.cppRegisterImage(cppPointer.pointer, name, imageHandle.handle)
    }

    /**
     * Unregister an image that was previously registered with [registerImage].
     * 
     * This removes the reference to the image from the CommandServer, allowing
     * the memory to be freed if the image handle was also deleted with [deleteImage].
     *
     * @param name The name of the referenced asset to unregister.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun unregisterImage(name: String) {
        bridge.cppUnregisterImage(cppPointer.pointer, name)
    }

    /**
     * Decode an audio file from the given bytes.
     * 
     * The decoded audio is stored on the CommandServer and can be used with
     * [registerAudio] to fulfill referenced assets in Rive files.
     *
     * @param bytes The bytes of the audio file to decode.
     * @return A handle to the decoded audio.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the audio could not be decoded.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun decodeAudio(bytes: ByteArray): AudioHandle {
        return suspendNativeRequest { requestID ->
            bridge.cppDecodeAudio(cppPointer.pointer, requestID, bytes)
        }
    }

    /**
     * Delete audio and free its resources.
     * 
     * Counterpart to [decodeAudio]. This is useful when you no longer need the
     * audio and want to free up memory.
     *
     * @param audioHandle The handle of the audio to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteAudio(audioHandle: AudioHandle) {
        bridge.cppDeleteAudio(cppPointer.pointer, audioHandle.handle)
    }

    /**
     * Register audio as an asset with the given name.
     * 
     * This allows the audio to be used to fulfill a referenced asset when loading
     * a Rive file. Registrations are global to this CommandQueue.
     *
     * @param name The name of the referenced asset to fulfill.
     * @param audioHandle The handle of the audio to register.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun registerAudio(name: String, audioHandle: AudioHandle) {
        bridge.cppRegisterAudio(cppPointer.pointer, name, audioHandle.handle)
    }

    /**
     * Unregister audio that was previously registered with [registerAudio].
     *
     * @param name The name of the referenced asset to unregister.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun unregisterAudio(name: String) {
        bridge.cppUnregisterAudio(cppPointer.pointer, name)
    }

    /**
     * Decode a font file from the given bytes.
     * 
     * The bytes should be for a font file such as TTF or OTF.
     * The decoded font is stored on the CommandServer and can be used with
     * [registerFont] to fulfill referenced assets in Rive files.
     *
     * @param bytes The bytes of the font file to decode.
     * @return A handle to the decoded font.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the font could not be decoded.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun decodeFont(bytes: ByteArray): FontHandle {
        return suspendNativeRequest { requestID ->
            bridge.cppDecodeFont(cppPointer.pointer, requestID, bytes)
        }
    }

    /**
     * Delete a font and free its resources.
     * 
     * Counterpart to [decodeFont]. This is useful when you no longer need the
     * font and want to free up memory.
     *
     * @param fontHandle The handle of the font to delete.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun deleteFont(fontHandle: FontHandle) {
        bridge.cppDeleteFont(cppPointer.pointer, fontHandle.handle)
    }

    /**
     * Register a font as an asset with the given name.
     * 
     * This allows the font to be used to fulfill a referenced asset when loading
     * a Rive file. Registrations are global to this CommandQueue.
     *
     * @param name The name of the referenced asset to fulfill.
     * @param fontHandle The handle of the font to register.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun registerFont(name: String, fontHandle: FontHandle) {
        bridge.cppRegisterFont(cppPointer.pointer, name, fontHandle.handle)
    }

    /**
     * Unregister a font that was previously registered with [registerFont].
     *
     * @param name The name of the referenced asset to unregister.
     * @throws IllegalStateException If the CommandQueue has been released.
     */
    @Throws(IllegalStateException::class)
    fun unregisterFont(name: String) {
        bridge.cppUnregisterFont(cppPointer.pointer, name)
    }

    // =============================================================================
    // JNI Callbacks (called from C++)
    // =============================================================================
    
    /**
     * Called from C++ when a file has been successfully loaded.
     * This resumes the suspended coroutine waiting for the file load.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param fileHandle The handle to the loaded file.
     */
    @Suppress("unused")  // Called from JNI
    private fun onFileLoaded(requestID: Long, fileHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<FileHandle>
            typedCont.resume(FileHandle(fileHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received file loaded callback for unknown requestID: $requestID" 
            }
        }
    }
    
    /**
     * Called from C++ when a file load has failed.
     * This resumes the suspended coroutine with an error.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onFileError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<FileHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("Failed to load file: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received file error callback for unknown requestID: $requestID - $error" 
            }
        }
    }
    
    /**
     * Called from C++ when a file has been deleted.
     * 
     * @param requestID The request ID (currently unused for delete operations).
     * @param fileHandle The handle of the deleted file.
     */
    @Suppress("unused")  // Called from JNI
    private fun onFileDeleted(requestID: Long, fileHandle: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "File deleted: handle=$fileHandle" }
    }
    
    /**
     * Called from C++ when artboard names have been retrieved.
     * This resumes the suspended coroutine waiting for the query.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param names The list of artboard names.
     */
    @Suppress("unused")  // Called from JNI
    private fun onArtboardNamesListed(requestID: Long, names: List<String>) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<List<String>>
            typedCont.resume(names)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received artboard names callback for unknown requestID: $requestID" 
            }
        }
    }
    
    /**
     * Called from C++ when state machine names have been retrieved.
     * This resumes the suspended coroutine waiting for the query.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param names The list of state machine names.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStateMachineNamesListed(requestID: Long, names: List<String>) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<List<String>>
            typedCont.resume(names)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received state machine names callback for unknown requestID: $requestID" 
            }
        }
    }
    
    /**
     * Called from C++ when view model names have been retrieved.
     * This resumes the suspended coroutine waiting for the query.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param names The list of view model names.
     */
    @Suppress("unused")  // Called from JNI
    private fun onViewModelNamesListed(requestID: Long, names: List<String>) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<List<String>>
            typedCont.resume(names)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received view model names callback for unknown requestID: $requestID" 
            }
        }
    }
    
    /**
     * Called from C++ when a query operation has failed.
     * This resumes the suspended coroutine with an error.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onQueryError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            // Query errors could be for any query type, so we need to handle generically
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Any>
            typedCont.resumeWithException(
                IllegalArgumentException("Query failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received query error callback for unknown requestID: $requestID - $error" 
            }
        }
    }
    
    /**
     * Called from C++ when an artboard has been successfully created.
     * This resumes the suspended coroutine waiting for the artboard.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param artboardHandle The handle to the created artboard.
     */
    @Suppress("unused")  // Called from JNI
    private fun onArtboardCreated(requestID: Long, artboardHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ArtboardHandle>
            typedCont.resume(ArtboardHandle(artboardHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received artboard created callback for unknown requestID: $requestID" 
            }
        }
    }
    
    /**
     * Called from C++ when an artboard creation has failed.
     * This resumes the suspended coroutine with an error.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onArtboardError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ArtboardHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("Artboard operation failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received artboard error callback for unknown requestID: $requestID - $error" 
            }
        }
    }
    
    /**
     * Called from C++ when an artboard has been deleted.
     * 
     * @param requestID The request ID (currently unused for delete operations).
     * @param artboardHandle The handle of the deleted artboard.
     */
    @Suppress("unused")  // Called from JNI
    private fun onArtboardDeleted(requestID: Long, artboardHandle: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Artboard deleted: handle=$artboardHandle" }
    }
    
    /**
     * Called from C++ when a state machine has been successfully created.
     * This resumes the suspended coroutine waiting for the state machine.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param smHandle The handle to the created state machine.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStateMachineCreated(requestID: Long, smHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<StateMachineHandle>
            typedCont.resume(StateMachineHandle(smHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received state machine created callback for unknown requestID: $requestID" 
            }
        }
    }
    
    /**
     * Called from C++ when a state machine creation has failed.
     * This resumes the suspended coroutine with an error.
     * 
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStateMachineError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<StateMachineHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("State machine operation failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) { 
                "Received state machine error callback for unknown requestID: $requestID - $error" 
            }
        }
    }
    
    /**
     * Called from C++ when a state machine has been deleted.
     * 
     * @param requestID The request ID (currently unused for delete operations).
     * @param smHandle The handle of the deleted state machine.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStateMachineDeleted(requestID: Long, smHandle: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "State machine deleted: handle=$smHandle" }
    }
    
    /**
     * Called from C++ when a state machine has settled.
     * This emits to the settledFlow.
     *
     * @param requestID The request ID (currently unused).
     * @param smHandle The handle of the settled state machine.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStateMachineSettled(requestID: Long, smHandle: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "State machine settled: handle=$smHandle" }
        _settledFlow.tryEmit(StateMachineHandle(smHandle))
    }

    // =============================================================================
    // JNI Callbacks for State Machine Inputs (Phase C.4)
    // =============================================================================

    /**
     * Called from C++ when input count has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param count The number of inputs.
     */
    @Suppress("unused")  // Called from JNI
    private fun onInputCountResult(requestID: Long, count: Int) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Int>
            typedCont.resume(count)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received input count callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when input names have been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param names The list of input names.
     */
    @Suppress("unused")  // Called from JNI
    private fun onInputNamesListed(requestID: Long, names: List<String>) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<List<String>>
            typedCont.resume(names)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received input names callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when input info has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param inputName The name of the input.
     * @param inputType The type of the input (as integer).
     */
    @Suppress("unused")  // Called from JNI
    private fun onInputInfoResult(requestID: Long, inputName: String, inputType: Int) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<InputInfo>
            typedCont.resume(InputInfo(inputName, InputType.fromValue(inputType)))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received input info callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a number input value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The value of the number input.
     */
    @Suppress("unused")  // Called from JNI
    private fun onNumberInputValue(requestID: Long, value: Float) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Float>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received number input value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a boolean input value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The value of the boolean input.
     */
    @Suppress("unused")  // Called from JNI
    private fun onBooleanInputValue(requestID: Long, value: Boolean) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Boolean>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received boolean input value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when an input set/fire operation has completed successfully.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onInputOperationSuccess(requestID: Long) {
        // Fire-and-forget operations don't have pending continuations
        RiveLog.d(COMMAND_QUEUE_TAG) { "Input operation succeeded: requestID=$requestID" }
    }

    /**
     * Called from C++ when an input operation has failed.
     *
     * @param requestID The request ID that identifies the waiting coroutine (if any).
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onInputOperationError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Any>
            typedCont.resumeWithException(
                IllegalArgumentException("Input operation failed: $error")
            )
        } else {
            // This may be an error for a fire-and-forget operation, just log it
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Input operation error (requestID=$requestID): $error"
            }
        }
    }

    // =============================================================================
    // JNI Callbacks for ViewModelInstance (Phase D.1)
    // =============================================================================

    /**
     * Called from C++ when a ViewModelInstance has been successfully created.
     * This resumes the suspended coroutine waiting for the VMI.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param vmiHandle The handle to the created ViewModelInstance.
     */
    @Suppress("unused")  // Called from JNI
    private fun onVMICreated(requestID: Long, vmiHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle>
            typedCont.resume(ViewModelInstanceHandle(vmiHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received VMI created callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a ViewModelInstance creation has failed.
     * This resumes the suspended coroutine with an error.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onVMIError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("ViewModelInstance operation failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received VMI error callback for unknown requestID: $requestID - $error"
            }
        }
    }

    /**
     * Called from C++ when a ViewModelInstance has been deleted.
     *
     * @param requestID The request ID (currently unused for delete operations).
     * @param vmiHandle The handle of the deleted ViewModelInstance.
     */
    @Suppress("unused")  // Called from JNI
    private fun onVMIDeleted(requestID: Long, vmiHandle: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "ViewModelInstance deleted: handle=$vmiHandle" }
    }

    // =============================================================================
    // JNI Callbacks for Property Operations (Phase D.2)
    // =============================================================================

    /**
     * Called from C++ when a number property value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The property value.
     */
    @Suppress("unused")  // Called from JNI
    private fun onNumberPropertyValue(requestID: Long, value: Float) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Float>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received number property value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a string property value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The property value.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStringPropertyValue(requestID: Long, value: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<String>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received string property value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a boolean property value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The property value.
     */
    @Suppress("unused")  // Called from JNI
    private fun onBooleanPropertyValue(requestID: Long, value: Boolean) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Boolean>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received boolean property value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a property operation has failed.
     *
     * @param requestID The request ID that identifies the waiting coroutine (if any).
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onPropertyError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Any>
            typedCont.resumeWithException(
                IllegalArgumentException("Property operation failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Property operation error (requestID=$requestID): $error"
            }
        }
    }

    /**
     * Called from C++ when a property set operation has succeeded.
     *
     * @param requestID The request ID (currently unused for fire-and-forget set operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onPropertySetSuccess(requestID: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Property set succeeded: requestID=$requestID" }
    }

    // =============================================================================
    // JNI Callbacks for Additional Property Types (Phase D.3)
    // =============================================================================

    /**
     * Called from C++ when an enum property value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The enum property value (as a string).
     */
    @Suppress("unused")  // Called from JNI
    private fun onEnumPropertyValue(requestID: Long, value: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<String>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received enum property value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a color property value has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param value The color property value in 0xAARRGGBB format.
     */
    @Suppress("unused")  // Called from JNI
    private fun onColorPropertyValue(requestID: Long, value: Int) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Int>
            typedCont.resume(value)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received color property value callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a trigger property has been successfully fired.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onTriggerFired(requestID: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Trigger fired: requestID=$requestID" }
    }

    // =============================================================================
    // JNI Callbacks for Property Subscriptions (Phase D.4)
    // =============================================================================

    /**
     * Called from C++ when a subscribed number property has been updated.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The new value.
     */
    @Suppress("unused")  // Called from JNI
    private fun onNumberPropertyUpdated(vmiHandle: Long, propertyPath: String, value: Float) {
        _numberPropertyFlow.tryEmit(
            PropertyUpdate(ViewModelInstanceHandle(vmiHandle), propertyPath, value)
        )
    }

    /**
     * Called from C++ when a subscribed string property has been updated.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The new value.
     */
    @Suppress("unused")  // Called from JNI
    private fun onStringPropertyUpdated(vmiHandle: Long, propertyPath: String, value: String) {
        _stringPropertyFlow.tryEmit(
            PropertyUpdate(ViewModelInstanceHandle(vmiHandle), propertyPath, value)
        )
    }

    /**
     * Called from C++ when a subscribed boolean property has been updated.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The new value.
     */
    @Suppress("unused")  // Called from JNI
    private fun onBooleanPropertyUpdated(vmiHandle: Long, propertyPath: String, value: Boolean) {
        _booleanPropertyFlow.tryEmit(
            PropertyUpdate(ViewModelInstanceHandle(vmiHandle), propertyPath, value)
        )
    }

    /**
     * Called from C++ when a subscribed enum property has been updated.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The new value (as string).
     */
    @Suppress("unused")  // Called from JNI
    private fun onEnumPropertyUpdated(vmiHandle: Long, propertyPath: String, value: String) {
        _enumPropertyFlow.tryEmit(
            PropertyUpdate(ViewModelInstanceHandle(vmiHandle), propertyPath, value)
        )
    }

    /**
     * Called from C++ when a subscribed color property has been updated.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the property.
     * @param value The new value (0xAARRGGBB format).
     */
    @Suppress("unused")  // Called from JNI
    private fun onColorPropertyUpdated(vmiHandle: Long, propertyPath: String, value: Int) {
        _colorPropertyFlow.tryEmit(
            PropertyUpdate(ViewModelInstanceHandle(vmiHandle), propertyPath, value)
        )
    }

    /**
     * Called from C++ when a subscribed trigger property has been fired.
     *
     * @param vmiHandle The handle of the ViewModelInstance.
     * @param propertyPath The path to the trigger property.
     */
    @Suppress("unused")  // Called from JNI
    private fun onTriggerPropertyFired(vmiHandle: Long, propertyPath: String) {
        _triggerPropertyFlow.tryEmit(
            PropertyUpdate(ViewModelInstanceHandle(vmiHandle), propertyPath, Unit)
        )
    }

    // =============================================================================
    // JNI Callbacks for List Operations (Phase D.5)
    // =============================================================================

    /**
     * Called from C++ when list size has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param size The number of items in the list.
     */
    @Suppress("unused")  // Called from JNI
    private fun onListSizeResult(requestID: Long, size: Int) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Int>
            typedCont.resume(size)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received list size callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a list item has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param itemHandle The handle of the list item (VMI).
     */
    @Suppress("unused")  // Called from JNI
    private fun onListItemResult(requestID: Long, itemHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle>
            typedCont.resume(ViewModelInstanceHandle(itemHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received list item callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a list operation has failed.
     *
     * @param requestID The request ID that identifies the waiting coroutine (if any).
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onListOperationError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Any>
            typedCont.resumeWithException(
                IllegalArgumentException("List operation failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "List operation error (requestID=$requestID): $error"
            }
        }
    }

    /**
     * Called from C++ when a list modification operation has succeeded.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onListOperationSuccess(requestID: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "List operation succeeded: requestID=$requestID" }
    }

    // =============================================================================
    // JNI Callbacks for Nested VMI Operations (Phase D.5)
    // =============================================================================

    /**
     * Called from C++ when a nested VMI property has been retrieved.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param nestedHandle The handle of the nested ViewModelInstance.
     */
    @Suppress("unused")  // Called from JNI
    private fun onInstancePropertyResult(requestID: Long, nestedHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle>
            typedCont.resume(ViewModelInstanceHandle(nestedHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received instance property callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a nested VMI operation has failed.
     *
     * @param requestID The request ID that identifies the waiting coroutine (if any).
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onInstancePropertyError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("Instance property operation failed: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Instance property error (requestID=$requestID): $error"
            }
        }
    }

    /**
     * Called from C++ when a nested VMI set operation has succeeded.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onInstancePropertySetSuccess(requestID: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Instance property set succeeded: requestID=$requestID" }
    }

    // =============================================================================
    // JNI Callbacks for Asset Property Operations (Phase D.5)
    // =============================================================================

    /**
     * Called from C++ when an asset property set operation has succeeded.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onAssetPropertySetSuccess(requestID: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) { "Asset property set succeeded: requestID=$requestID" }
    }

    /**
     * Called from C++ when an asset property operation has failed.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onAssetPropertyError(requestID: Long, error: String) {
        RiveLog.w(COMMAND_QUEUE_TAG) {
            "Asset property error (requestID=$requestID): $error"
        }
    }

    // =============================================================================
    // Phase D.6: VMI Binding Callbacks
    // =============================================================================

    /**
     * Called from C++ when VMI binding has succeeded.
     * This is a fire-and-forget operation, so we just log success.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     */
    @Suppress("unused")  // Called from JNI
    private fun onVMIBindingSuccess(requestID: Long) {
        RiveLog.d(COMMAND_QUEUE_TAG) {
            "VMI binding succeeded (requestID=$requestID)"
        }
    }

    /**
     * Called from C++ when VMI binding has failed.
     *
     * @param requestID The request ID (currently unused for fire-and-forget operations).
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onVMIBindingError(requestID: Long, error: String) {
        RiveLog.w(COMMAND_QUEUE_TAG) {
            "VMI binding error (requestID=$requestID): $error"
        }
    }

    /**
     * Called from C++ with the result of a default VMI query.
     * Resumes the suspended coroutine with the VMI handle (or null if 0).
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param vmiHandle The handle of the default VMI, or 0 if no default VMI exists.
     */
    @Suppress("unused")  // Called from JNI
    private fun onDefaultVMIResult(requestID: Long, vmiHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle?>
            val result = if (vmiHandle == 0L) null else ViewModelInstanceHandle(vmiHandle)
            typedCont.resume(result)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received default VMI result callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when a default VMI query has failed.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onDefaultVMIError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ViewModelInstanceHandle?>
            typedCont.resumeWithException(IllegalArgumentException(error))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received default VMI error callback for unknown requestID: $requestID"
            }
        }
    }

    // =========================================================================
    // Phase C.2.3: Render Target Operations - JNI Callbacks
    // =========================================================================

    /**
     * Called from JNI when a render target is successfully created.
     */
    @Suppress("unused")  // Called from JNI
    private fun onRenderTargetCreated(requestID: Long, renderTargetHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Long>
            typedCont.resume(renderTargetHandle)
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received render target created callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from JNI when render target creation fails.
     */
    @Suppress("unused")  // Called from JNI
    private fun onRenderTargetError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<Long>
            typedCont.resumeWithException(IllegalArgumentException(error))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received render target error callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from JNI when a render target is successfully deleted.
     */
    @Suppress("unused")  // Called from JNI
    private fun onRenderTargetDeleted(requestID: Long) {
        // Fire-and-forget operation, no continuation to resume
        RiveLog.d(COMMAND_QUEUE_TAG) {
            "Render target deleted successfully (requestID: $requestID)"
        }
    }

    // =============================================================================
    // JNI Callbacks for Asset Operations (Phase E.1)
    // =============================================================================

    /**
     * Called from C++ when an image has been successfully decoded.
     * This resumes the suspended coroutine waiting for the image.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param imageHandle The handle to the decoded image.
     */
    @Suppress("unused")  // Called from JNI
    private fun onImageDecoded(requestID: Long, imageHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ImageHandle>
            typedCont.resume(ImageHandle(imageHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received image decoded callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when image decoding has failed.
     * This resumes the suspended coroutine with an error.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onImageError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<ImageHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("Failed to decode image: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received image error callback for unknown requestID: $requestID - $error"
            }
        }
    }

    /**
     * Called from C++ when audio has been successfully decoded.
     * This resumes the suspended coroutine waiting for the audio.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param audioHandle The handle to the decoded audio.
     */
    @Suppress("unused")  // Called from JNI
    private fun onAudioDecoded(requestID: Long, audioHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<AudioHandle>
            typedCont.resume(AudioHandle(audioHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received audio decoded callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when audio decoding has failed.
     * This resumes the suspended coroutine with an error.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onAudioError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<AudioHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("Failed to decode audio: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received audio error callback for unknown requestID: $requestID - $error"
            }
        }
    }

    /**
     * Called from C++ when a font has been successfully decoded.
     * This resumes the suspended coroutine waiting for the font.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param fontHandle The handle to the decoded font.
     */
    @Suppress("unused")  // Called from JNI
    private fun onFontDecoded(requestID: Long, fontHandle: Long) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<FontHandle>
            typedCont.resume(FontHandle(fontHandle))
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received font decoded callback for unknown requestID: $requestID"
            }
        }
    }

    /**
     * Called from C++ when font decoding has failed.
     * This resumes the suspended coroutine with an error.
     *
     * @param requestID The request ID that identifies the waiting coroutine.
     * @param error The error message.
     */
    @Suppress("unused")  // Called from JNI
    private fun onFontError(requestID: Long, error: String) {
        val continuation = pendingContinuations.remove(requestID)
        if (continuation != null) {
            @Suppress("UNCHECKED_CAST")
            val typedCont = continuation as CancellableContinuation<FontHandle>
            typedCont.resumeWithException(
                IllegalArgumentException("Failed to decode font: $error")
            )
        } else {
            RiveLog.w(COMMAND_QUEUE_TAG) {
                "Received font error callback for unknown requestID: $requestID - $error"
            }
        }
    }
}