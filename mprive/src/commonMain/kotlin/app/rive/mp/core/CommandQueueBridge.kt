package app.rive.mp.core

import app.rive.mp.CommandQueue
import app.rive.mp.RiveInitializationException

/**
 * Abstraction of calls to the native command queue.
 *
 * This interface allows for:
 * 1. Platform-specific implementations (JNI on Android, JNA on Desktop, etc.)
 * 2. Mocking in tests
 * 3. Better separation of concerns between Kotlin and native code
 *
 * Each platform provides its own implementation:
 * - Android: CommandQueueJNIBridge (uses JNI external functions)
 * - Desktop: CommandQueueJNABridge (future - uses JNA)
 * - iOS: CommandQueueCInteropBridge (future - uses C Interop)
 */
interface CommandQueueBridge {
    // =========================================================================
    // Core Lifecycle
    // =========================================================================
    
    /**
     * Construct a new CommandQueue native object.
     * @param renderContextPointer Pointer to the RenderContext native object.
     * @return Pointer to the created CommandQueue native object.
     * @throws RiveInitializationException If the command queue cannot be created.
     */
    @Throws(RiveInitializationException::class)
    fun cppConstructor(renderContextPointer: Long): Long
    
    /**
     * Delete a CommandQueue native object.
     * @param pointer Pointer to the CommandQueue to delete.
     */
    fun cppDelete(pointer: Long)
    
    /**
     * Create listeners for callbacks from the native layer.
     * @param pointer Pointer to the CommandQueue.
     * @param receiver The CommandQueue instance to receive callbacks.
     * @return The created Listeners object.
     */
    fun cppCreateListeners(pointer: Long, receiver: CommandQueue): Listeners
    
    /**
     * Poll messages from the CommandServer.
     * @param pointer Pointer to the CommandQueue.
     * @param receiver The CommandQueue instance to receive callbacks.
     */
    fun cppPollMessages(pointer: Long, receiver: CommandQueue)
    
    // =========================================================================
    // File Operations
    // =========================================================================
    
    fun cppLoadFile(pointer: Long, requestID: Long, bytes: ByteArray)
    fun cppDeleteFile(pointer: Long, requestID: Long, fileHandle: Long)
    fun cppGetArtboardNames(pointer: Long, requestID: Long, fileHandle: Long)
    fun cppGetStateMachineNames(pointer: Long, requestID: Long, artboardHandle: Long)
    fun cppGetViewModelNames(pointer: Long, requestID: Long, fileHandle: Long)
    
    // =========================================================================
    // File Introspection APIs (Phase E.2)
    // =========================================================================
    
    /**
     * Get the names of all ViewModel instances for a given ViewModel in a file.
     * @param pointer Pointer to the CommandQueue.
     * @param requestID The request ID for async callback.
     * @param fileHandle The handle of the file to query.
     * @param viewModelName The name of the ViewModel to query instances for.
     */
    fun cppGetViewModelInstanceNames(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String)
    
    /**
     * Get the properties defined on a ViewModel in a file.
     * @param pointer Pointer to the CommandQueue.
     * @param requestID The request ID for async callback.
     * @param fileHandle The handle of the file to query.
     * @param viewModelName The name of the ViewModel to query properties for.
     */
    fun cppGetViewModelProperties(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String)
    
    /**
     * Get all enum definitions in a file.
     * @param pointer Pointer to the CommandQueue.
     * @param requestID The request ID for async callback.
     * @param fileHandle The handle of the file to query.
     */
    fun cppGetEnums(pointer: Long, requestID: Long, fileHandle: Long)
    
    // =========================================================================
    // Artboard Operations (SYNCHRONOUS - returns handle directly)
    // =========================================================================
    
    fun cppCreateDefaultArtboard(pointer: Long, requestID: Long, fileHandle: Long): Long
    fun cppCreateArtboardByName(pointer: Long, requestID: Long, fileHandle: Long, name: String): Long
    fun cppDeleteArtboard(pointer: Long, requestID: Long, artboardHandle: Long)
    fun cppResizeArtboard(pointer: Long, artboardHandle: Long, width: Int, height: Int, scaleFactor: Float)
    fun cppResetArtboardSize(pointer: Long, artboardHandle: Long)
    
    // =========================================================================
    // State Machine Operations (SYNCHRONOUS for creation)
    // =========================================================================
    
    fun cppCreateDefaultStateMachine(pointer: Long, requestID: Long, artboardHandle: Long): Long
    fun cppCreateStateMachineByName(pointer: Long, requestID: Long, artboardHandle: Long, name: String): Long
    fun cppDeleteStateMachine(pointer: Long, requestID: Long, stateMachineHandle: Long)
    fun cppAdvanceStateMachine(pointer: Long, stateMachineHandle: Long, deltaTimeNs: Long)
    
    // =========================================================================
    // State Machine Input Manipulation (SMI - Legacy Support for RiveSprite)
    // =========================================================================
    
    fun cppSetStateMachineNumberInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Float)
    fun cppSetStateMachineBooleanInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Boolean)
    fun cppFireStateMachineTrigger(pointer: Long, stateMachineHandle: Long, inputName: String)
    
    // =========================================================================
    // State Machine Input Query (Phase C.4)
    // =========================================================================
    
    fun cppGetInputCount(pointer: Long, requestID: Long, smHandle: Long)
    fun cppGetInputNames(pointer: Long, requestID: Long, smHandle: Long)
    fun cppGetInputInfo(pointer: Long, requestID: Long, smHandle: Long, inputIndex: Int)
    fun cppGetNumberInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String)
    fun cppSetNumberInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String, value: Float)
    fun cppGetBooleanInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String)
    fun cppSetBooleanInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String, value: Boolean)
    fun cppFireTrigger(pointer: Long, requestID: Long, smHandle: Long, inputName: String)
    
    // =========================================================================
    // ViewModelInstance Operations (SYNCHRONOUS for creation)
    // =========================================================================
    
    fun cppCreateBlankVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String): Long
    fun cppCreateDefaultVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String): Long
    fun cppCreateNamedVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String, instanceName: String): Long
    fun cppDeleteVMI(pointer: Long, requestID: Long, vmiHandle: Long)
    fun cppBindViewModelInstance(pointer: Long, requestID: Long, smHandle: Long, vmiHandle: Long)
    fun cppGetDefaultViewModelInstance(pointer: Long, requestID: Long, fileHandle: Long, artboardHandle: Long)
    
    // =========================================================================
    // Property Operations
    // =========================================================================
    
    fun cppGetNumberProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppSetNumberProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Float)
    fun cppGetStringProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppSetStringProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: String)
    fun cppGetBooleanProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppSetBooleanProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Boolean)
    fun cppGetEnumProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppSetEnumProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: String)
    fun cppGetColorProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppSetColorProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Int)
    fun cppFireTriggerProperty(pointer: Long, vmiHandle: Long, propertyPath: String)
    
    // =========================================================================
    // Property Subscriptions
    // =========================================================================
    
    fun cppSubscribeToProperty(pointer: Long, vmiHandle: Long, propertyPath: String, propertyType: Int)
    fun cppUnsubscribeFromProperty(pointer: Long, vmiHandle: Long, propertyPath: String, propertyType: Int)
    
    // =========================================================================
    // List Operations
    // =========================================================================
    
    fun cppGetListSize(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppGetListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int)
    fun cppAddListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long)
    fun cppAddListItemAt(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int, itemHandle: Long)
    fun cppRemoveListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long)
    fun cppRemoveListItemAt(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int)
    fun cppSwapListItems(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, indexA: Int, indexB: Int)
    
    // =========================================================================
    // Nested VMI Operations
    // =========================================================================
    
    fun cppGetInstanceProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    fun cppSetInstanceProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, nestedHandle: Long)
    
    // =========================================================================
    // Asset Property Operations
    // =========================================================================
    
    fun cppSetImageProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, imageHandle: Long)
    fun cppSetArtboardProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, fileHandle: Long, artboardHandle: Long)
    
    // =========================================================================
    // Pointer Events
    // =========================================================================
    
    fun cppPointerMove(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    fun cppPointerDown(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    fun cppPointerUp(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    fun cppPointerExit(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    
    // =========================================================================
    // Render Target Operations
    // =========================================================================
    
    fun cppCreateRenderTarget(pointer: Long, requestID: Long, width: Int, height: Int, sampleCount: Int)
    fun cppDeleteRenderTarget(pointer: Long, requestID: Long, renderTargetHandle: Long)
    fun cppCreateRiveRenderTarget(pointer: Long, width: Int, height: Int): Long
    fun cppCreateDrawKey(pointer: Long): Long
    
    // =========================================================================
    // Drawing Operations
    // =========================================================================
    
    fun cppDraw(
        pointer: Long,
        renderContextPointer: Long,
        surfaceNativePointer: Long,
        drawKey: Long,
        artboardHandle: Long,
        stateMachineHandle: Long,
        renderTargetPointer: Long,
        width: Int,
        height: Int,
        fit: Byte,
        alignment: Byte,
        scaleFactor: Float,
        clearColor: Int
    )
    
    fun cppDrawToBuffer(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, artboardHandle: Long, stateMachineHandle: Long, renderTargetPointer: Long, width: Int, height: Int, fit: Byte, alignment: Byte, scaleFactor: Float, clearColor: Int, buffer: ByteArray)
    
    fun cppRunOnCommandServer(pointer: Long, work: () -> Unit)
    
    // =========================================================================
    // Batch Sprite Rendering (RiveSpriteScene support)
    // =========================================================================
    
    fun cppDrawMultiple(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, renderTargetPointer: Long, viewportWidth: Int, viewportHeight: Int, clearColor: Int, artboardHandles: LongArray, stateMachineHandles: LongArray, transforms: FloatArray, artboardWidths: FloatArray, artboardHeights: FloatArray, count: Int)
    
    fun cppDrawMultipleToBuffer(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, renderTargetPointer: Long, viewportWidth: Int, viewportHeight: Int, clearColor: Int, artboardHandles: LongArray, stateMachineHandles: LongArray, transforms: FloatArray, artboardWidths: FloatArray, artboardHeights: FloatArray, count: Int, buffer: ByteArray)
    
    // =========================================================================
    // Asset Operations (Phase E.1)
    // =========================================================================
    
    /**
     * Decode an image from bytes.
     * @param pointer Pointer to the CommandQueue.
     * @param requestID The request ID for async callback.
     * @param bytes The image file bytes (PNG, JPEG, etc.).
     */
    fun cppDecodeImage(pointer: Long, requestID: Long, bytes: ByteArray)
    
    /**
     * Delete an image and free its resources.
     * @param pointer Pointer to the CommandQueue.
     * @param imageHandle The handle of the image to delete.
     */
    fun cppDeleteImage(pointer: Long, imageHandle: Long)
    
    /**
     * Register an image as an asset with a name for use in Rive files.
     * @param pointer Pointer to the CommandQueue.
     * @param name The name to register the image under.
     * @param imageHandle The handle of the image to register.
     */
    fun cppRegisterImage(pointer: Long, name: String, imageHandle: Long)
    
    /**
     * Unregister an image asset by name.
     * @param pointer Pointer to the CommandQueue.
     * @param name The name of the image to unregister.
     */
    fun cppUnregisterImage(pointer: Long, name: String)
    
    /**
     * Decode audio from bytes.
     * @param pointer Pointer to the CommandQueue.
     * @param requestID The request ID for async callback.
     * @param bytes The audio file bytes.
     */
    fun cppDecodeAudio(pointer: Long, requestID: Long, bytes: ByteArray)
    
    /**
     * Delete audio and free its resources.
     * @param pointer Pointer to the CommandQueue.
     * @param audioHandle The handle of the audio to delete.
     */
    fun cppDeleteAudio(pointer: Long, audioHandle: Long)
    
    /**
     * Register audio as an asset with a name for use in Rive files.
     * @param pointer Pointer to the CommandQueue.
     * @param name The name to register the audio under.
     * @param audioHandle The handle of the audio to register.
     */
    fun cppRegisterAudio(pointer: Long, name: String, audioHandle: Long)
    
    /**
     * Unregister an audio asset by name.
     * @param pointer Pointer to the CommandQueue.
     * @param name The name of the audio to unregister.
     */
    fun cppUnregisterAudio(pointer: Long, name: String)
    
    /**
     * Decode a font from bytes.
     * @param pointer Pointer to the CommandQueue.
     * @param requestID The request ID for async callback.
     * @param bytes The font file bytes (TTF, OTF, etc.).
     */
    fun cppDecodeFont(pointer: Long, requestID: Long, bytes: ByteArray)
    
    /**
     * Delete a font and free its resources.
     * @param pointer Pointer to the CommandQueue.
     * @param fontHandle The handle of the font to delete.
     */
    fun cppDeleteFont(pointer: Long, fontHandle: Long)
    
    /**
     * Register a font as an asset with a name for use in Rive files.
     * @param pointer Pointer to the CommandQueue.
     * @param name The name to register the font under.
     * @param fontHandle The handle of the font to register.
     */
    fun cppRegisterFont(pointer: Long, name: String, fontHandle: Long)
    
    /**
     * Unregister a font asset by name.
     * @param pointer Pointer to the CommandQueue.
     * @param name The name of the font to unregister.
     */
    fun cppUnregisterFont(pointer: Long, name: String)
}

/**
 * expect/actual pattern for creating the platform-specific bridge.
 * Each platform provides its own implementation.
 */
expect fun createCommandQueueBridge(): CommandQueueBridge