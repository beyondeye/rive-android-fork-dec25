package app.rive.mp

import kotlinx.atomicfu.atomic
import kotlinx.coroutines.CancellableContinuation
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.cancellation.CancellationException
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

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
    private val renderContext: RenderContext = createDefaultRenderContext()
) : RefCounted {
    
    // External JNI method declarations
    private external fun cppConstructor(renderContextPtr: Long): Long
    private external fun cppDelete(ptr: Long)
    private external fun cppPollMessages(ptr: Long)
    
    // Phase B methods
    private external fun cppLoadFile(ptr: Long, requestID: Long, bytes: ByteArray)
    private external fun cppDeleteFile(ptr: Long, requestID: Long, fileHandle: Long)
    private external fun cppGetArtboardNames(ptr: Long, requestID: Long, fileHandle: Long)
    private external fun cppGetStateMachineNames(ptr: Long, requestID: Long, artboardHandle: Long)
    private external fun cppGetViewModelNames(ptr: Long, requestID: Long, fileHandle: Long)
    private external fun cppCreateDefaultArtboard(ptr: Long, requestID: Long, fileHandle: Long)
    private external fun cppCreateArtboardByName(ptr: Long, requestID: Long, fileHandle: Long, name: String)
    private external fun cppDeleteArtboard(ptr: Long, requestID: Long, artboardHandle: Long)
    
    companion object {
        /**
         * Maximum number of concurrent subscribers that can safely use this CommandQueue.
         */
        const val MAX_CONCURRENT_SUBSCRIBERS = 32
    }
    
    /**
     * The native pointer to the CommandServer C++ object, held in a reference-counted pointer.
     */
    private val cppPointer = RCPointer(
        cppConstructor(renderContext.nativeObjectPointer),
        COMMAND_QUEUE_TAG,
        ::dispose
    )
    
    /**
     * Cleanup to be performed when the ref count reaches 0.
     *
     * Any pending continuations are cancelled to avoid callers hanging indefinitely.
     */
    private fun dispose(cppPointer: Long) {
        cppDelete(cppPointer)
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
    fun pollMessages() = cppPollMessages(cppPointer.pointer)
    
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
     * Phase A stub - to be implemented in Phase C.
     */
    fun createRiveRenderTarget(width: Int, height: Int): Long {
        // Placeholder for Phase A
        return 0L
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
            cppLoadFile(cppPointer.pointer, requestID, bytes)
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
        cppDeleteFile(cppPointer.pointer, requestID, fileHandle.handle)
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
            cppGetArtboardNames(cppPointer.pointer, requestID, fileHandle.handle)
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
            cppGetStateMachineNames(cppPointer.pointer, requestID, artboardHandle.handle)
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
            cppGetViewModelNames(cppPointer.pointer, requestID, fileHandle.handle)
        }
    }
    
    /**
     * Create the default artboard from a file.
     * 
     * @param fileHandle The handle of the file to create artboard from.
     * @return A handle to the created artboard.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid or artboard creation fails.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createDefaultArtboard(fileHandle: FileHandle): ArtboardHandle {
        return suspendNativeRequest { requestID ->
            cppCreateDefaultArtboard(cppPointer.pointer, requestID, fileHandle.handle)
        }
    }
    
    /**
     * Create an artboard by name from a file.
     * 
     * @param fileHandle The handle of the file to create artboard from.
     * @param name The name of the artboard to create.
     * @return A handle to the created artboard.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid or artboard not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createArtboardByName(fileHandle: FileHandle, name: String): ArtboardHandle {
        return suspendNativeRequest { requestID ->
            cppCreateArtboardByName(cppPointer.pointer, requestID, fileHandle.handle, name)
        }
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
        cppDeleteArtboard(cppPointer.pointer, requestID, artboardHandle.handle)
    }
    
    // =============================================================================
    // Phase C: State Machine Operations
    // =============================================================================
    
    // External JNI methods for state machines
    private external fun cppCreateDefaultStateMachine(ptr: Long, requestID: Long, artboardHandle: Long)
    private external fun cppCreateStateMachineByName(ptr: Long, requestID: Long, artboardHandle: Long, name: String)
    private external fun cppAdvanceStateMachine(ptr: Long, requestID: Long, smHandle: Long, deltaTimeSeconds: Float)
    private external fun cppDeleteStateMachine(ptr: Long, requestID: Long, smHandle: Long)

    // External JNI methods for state machine inputs (Phase C.4)
    private external fun cppGetInputCount(ptr: Long, requestID: Long, smHandle: Long)
    private external fun cppGetInputNames(ptr: Long, requestID: Long, smHandle: Long)
    private external fun cppGetInputInfo(ptr: Long, requestID: Long, smHandle: Long, inputIndex: Int)
    private external fun cppGetNumberInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String)
    private external fun cppSetNumberInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String, value: Float)
    private external fun cppGetBooleanInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String)
    private external fun cppSetBooleanInput(ptr: Long, requestID: Long, smHandle: Long, inputName: String, value: Boolean)
    private external fun cppFireTrigger(ptr: Long, requestID: Long, smHandle: Long, inputName: String)

    // External JNI methods for ViewModelInstance (Phase D.1)
    private external fun cppCreateBlankVMI(ptr: Long, requestID: Long, fileHandle: Long, viewModelName: String)
    private external fun cppCreateDefaultVMI(ptr: Long, requestID: Long, fileHandle: Long, viewModelName: String)
    private external fun cppCreateNamedVMI(ptr: Long, requestID: Long, fileHandle: Long, viewModelName: String, instanceName: String)
    private external fun cppDeleteVMI(ptr: Long, requestID: Long, vmiHandle: Long)

    // External JNI methods for Property Operations (Phase D.2)
    private external fun cppGetNumberProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    private external fun cppSetNumberProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: Float)
    private external fun cppGetStringProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    private external fun cppSetStringProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: String)
    private external fun cppGetBooleanProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    private external fun cppSetBooleanProperty(ptr: Long, requestID: Long, vmiHandle: Long, propertyPath: String, value: Boolean)
    
    /**
     * Create the default state machine from an artboard.
     * 
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @return A handle to the created state machine.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the artboard handle is invalid or state machine creation fails.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createDefaultStateMachine(artboardHandle: ArtboardHandle): StateMachineHandle {
        return suspendNativeRequest { requestID ->
            cppCreateDefaultStateMachine(cppPointer.pointer, requestID, artboardHandle.handle)
        }
    }
    
    /**
     * Create a state machine by name from an artboard.
     * 
     * @param artboardHandle The handle of the artboard to create state machine from.
     * @param name The name of the state machine to create.
     * @return A handle to the created state machine.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the artboard handle is invalid or state machine not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createStateMachineByName(artboardHandle: ArtboardHandle, name: String): StateMachineHandle {
        return suspendNativeRequest { requestID ->
            cppCreateStateMachineByName(cppPointer.pointer, requestID, artboardHandle.handle, name)
        }
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
        val requestID = nextRequestID.getAndIncrement()
        cppAdvanceStateMachine(cppPointer.pointer, requestID, smHandle.handle, deltaTimeSeconds)
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
        cppDeleteStateMachine(cppPointer.pointer, requestID, smHandle.handle)
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
            cppGetInputCount(cppPointer.pointer, requestID, smHandle.handle)
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
            cppGetInputNames(cppPointer.pointer, requestID, smHandle.handle)
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
            cppGetInputInfo(cppPointer.pointer, requestID, smHandle.handle, inputIndex)
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
            cppGetNumberInput(cppPointer.pointer, requestID, smHandle.handle, inputName)
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
        cppSetNumberInput(cppPointer.pointer, requestID, smHandle.handle, inputName, value)
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
            cppGetBooleanInput(cppPointer.pointer, requestID, smHandle.handle, inputName)
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
        cppSetBooleanInput(cppPointer.pointer, requestID, smHandle.handle, inputName, value)
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
        cppFireTrigger(cppPointer.pointer, requestID, smHandle.handle, inputName)
    }

    // =============================================================================
    // Phase D.1: ViewModelInstance Creation
    // =============================================================================

    /**
     * Create a blank ViewModelInstance from a named ViewModel.
     * A blank instance has all properties set to their default values.
     *
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to create an instance of.
     * @return A handle to the created ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid or ViewModel not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createBlankViewModelInstance(
        fileHandle: FileHandle,
        viewModelName: String
    ): ViewModelInstanceHandle {
        return suspendNativeRequest { requestID ->
            cppCreateBlankVMI(cppPointer.pointer, requestID, fileHandle.handle, viewModelName)
        }
    }

    /**
     * Create a default ViewModelInstance from a named ViewModel.
     * A default instance uses the ViewModel's default instance values if available.
     *
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to create an instance of.
     * @return A handle to the created ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid or ViewModel not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createDefaultViewModelInstance(
        fileHandle: FileHandle,
        viewModelName: String
    ): ViewModelInstanceHandle {
        return suspendNativeRequest { requestID ->
            cppCreateDefaultVMI(cppPointer.pointer, requestID, fileHandle.handle, viewModelName)
        }
    }

    /**
     * Create a named ViewModelInstance from a named ViewModel.
     * Uses the values defined for the named instance in the Rive file.
     *
     * @param fileHandle The handle of the file containing the ViewModel.
     * @param viewModelName The name of the ViewModel to create an instance of.
     * @param instanceName The name of the specific instance to create.
     * @return A handle to the created ViewModelInstance.
     * @throws IllegalStateException If the CommandQueue has been released.
     * @throws CancellationException If the operation is cancelled.
     * @throws IllegalArgumentException If the file handle is invalid, ViewModel not found, or instance not found.
     */
    @Throws(IllegalStateException::class, CancellationException::class, IllegalArgumentException::class)
    suspend fun createNamedViewModelInstance(
        fileHandle: FileHandle,
        viewModelName: String,
        instanceName: String
    ): ViewModelInstanceHandle {
        return suspendNativeRequest { requestID ->
            cppCreateNamedVMI(cppPointer.pointer, requestID, fileHandle.handle, viewModelName, instanceName)
        }
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
        cppDeleteVMI(cppPointer.pointer, requestID, vmiHandle.handle)
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
            cppGetNumberProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
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
        cppSetNumberProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, value)
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
            cppGetStringProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
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
        cppSetStringProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, value)
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
            cppGetBooleanProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath)
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
        cppSetBooleanProperty(cppPointer.pointer, requestID, vmiHandle.handle, propertyPath, value)
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
}
