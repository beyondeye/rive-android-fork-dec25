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
}
