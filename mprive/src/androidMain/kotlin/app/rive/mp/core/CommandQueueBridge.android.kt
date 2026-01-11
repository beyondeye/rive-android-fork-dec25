package app.rive.mp.core

import app.rive.mp.CommandQueue
import app.rive.mp.RiveInitializationException

/**
 * Android JNI implementation of [CommandQueueBridge].
 * All methods are declared as external and link to native C++ code via JNI.
 */
internal class CommandQueueJNIBridge : CommandQueueBridge {
    // =========================================================================
    // Core Lifecycle
    // =========================================================================
    
    @Throws(RiveInitializationException::class)
    external override fun cppConstructor(renderContextPointer: Long): Long
    external override fun cppDelete(pointer: Long)
    external override fun cppCreateListeners(pointer: Long, receiver: CommandQueue): Listeners
    external override fun cppPollMessages(pointer: Long, receiver: CommandQueue)
    
    // =========================================================================
    // File Operations
    // =========================================================================
    
    external override fun cppLoadFile(pointer: Long, requestID: Long, bytes: ByteArray)
    external override fun cppDeleteFile(pointer: Long, requestID: Long, fileHandle: Long)
    external override fun cppGetArtboardNames(pointer: Long, requestID: Long, fileHandle: Long)
    external override fun cppGetStateMachineNames(pointer: Long, requestID: Long, artboardHandle: Long)
    external override fun cppGetViewModelNames(pointer: Long, requestID: Long, fileHandle: Long)
    
    // =========================================================================
    // Artboard Operations (SYNCHRONOUS)
    // =========================================================================
    
    external override fun cppCreateDefaultArtboard(pointer: Long, requestID: Long, fileHandle: Long): Long
    external override fun cppCreateArtboardByName(pointer: Long, requestID: Long, fileHandle: Long, name: String): Long
    external override fun cppDeleteArtboard(pointer: Long, requestID: Long, artboardHandle: Long)
    external override fun cppResizeArtboard(pointer: Long, artboardHandle: Long, width: Int, height: Int, scaleFactor: Float)
    external override fun cppResetArtboardSize(pointer: Long, artboardHandle: Long)
    
    // =========================================================================
    // State Machine Operations (SYNCHRONOUS for creation)
    // =========================================================================
    
    external override fun cppCreateDefaultStateMachine(pointer: Long, requestID: Long, artboardHandle: Long): Long
    external override fun cppCreateStateMachineByName(pointer: Long, requestID: Long, artboardHandle: Long, name: String): Long
    external override fun cppDeleteStateMachine(pointer: Long, requestID: Long, stateMachineHandle: Long)
    external override fun cppAdvanceStateMachine(pointer: Long, stateMachineHandle: Long, deltaTimeNs: Long)
    
    // =========================================================================
    // State Machine Input Manipulation (SMI)
    // =========================================================================
    
    external override fun cppSetStateMachineNumberInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Float)
    external override fun cppSetStateMachineBooleanInput(pointer: Long, stateMachineHandle: Long, inputName: String, value: Boolean)
    external override fun cppFireStateMachineTrigger(pointer: Long, stateMachineHandle: Long, inputName: String)
    
    // =========================================================================
    // State Machine Input Query
    // =========================================================================
    
    external override fun cppGetInputCount(pointer: Long, requestID: Long, smHandle: Long)
    external override fun cppGetInputNames(pointer: Long, requestID: Long, smHandle: Long)
    external override fun cppGetInputInfo(pointer: Long, requestID: Long, smHandle: Long, inputIndex: Int)
    external override fun cppGetNumberInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String)
    external override fun cppSetNumberInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String, value: Float)
    external override fun cppGetBooleanInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String)
    external override fun cppSetBooleanInput(pointer: Long, requestID: Long, smHandle: Long, inputName: String, value: Boolean)
    external override fun cppFireTrigger(pointer: Long, requestID: Long, smHandle: Long, inputName: String)
    
    // =========================================================================
    // ViewModelInstance Operations (SYNCHRONOUS for creation)
    // =========================================================================
    
    external override fun cppCreateBlankVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String): Long
    external override fun cppCreateDefaultVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String): Long
    external override fun cppCreateNamedVMI(pointer: Long, requestID: Long, fileHandle: Long, viewModelName: String, instanceName: String): Long
    external override fun cppDeleteVMI(pointer: Long, requestID: Long, vmiHandle: Long)
    external override fun cppBindViewModelInstance(pointer: Long, requestID: Long, smHandle: Long, vmiHandle: Long)
    external override fun cppGetDefaultViewModelInstance(pointer: Long, requestID: Long, fileHandle: Long, artboardHandle: Long)
    
    // =========================================================================
    // Property Operations
    // =========================================================================
    
    external override fun cppGetNumberProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppSetNumberProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Float)
    external override fun cppGetStringProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppSetStringProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: String)
    external override fun cppGetBooleanProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppSetBooleanProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Boolean)
    external override fun cppGetEnumProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppSetEnumProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: String)
    external override fun cppGetColorProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppSetColorProperty(pointer: Long, vmiHandle: Long, propertyPath: String, value: Int)
    external override fun cppFireTriggerProperty(pointer: Long, vmiHandle: Long, propertyPath: String)
    
    // =========================================================================
    // Property Subscriptions
    // =========================================================================
    
    external override fun cppSubscribeToProperty(pointer: Long, vmiHandle: Long, propertyPath: String, propertyType: Int)
    external override fun cppUnsubscribeFromProperty(pointer: Long, vmiHandle: Long, propertyPath: String, propertyType: Int)
    
    // =========================================================================
    // List Operations
    // =========================================================================
    
    external override fun cppGetListSize(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppGetListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int)
    external override fun cppAddListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long)
    external override fun cppAddListItemAt(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int, itemHandle: Long)
    external override fun cppRemoveListItem(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, itemHandle: Long)
    external override fun cppRemoveListItemAt(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, index: Int)
    external override fun cppSwapListItems(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, indexA: Int, indexB: Int)
    
    // =========================================================================
    // Nested VMI Operations
    // =========================================================================
    
    external override fun cppGetInstanceProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String)
    external override fun cppSetInstanceProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, nestedHandle: Long)
    
    // =========================================================================
    // Asset Property Operations
    // =========================================================================
    
    external override fun cppSetImageProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, imageHandle: Long)
    external override fun cppSetArtboardProperty(pointer: Long, requestID: Long, vmiHandle: Long, propertyPath: String, fileHandle: Long, artboardHandle: Long)
    
    // =========================================================================
    // Pointer Events
    // =========================================================================
    
    external override fun cppPointerMove(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    external override fun cppPointerDown(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    external override fun cppPointerUp(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    external override fun cppPointerExit(pointer: Long, stateMachineHandle: Long, fit: Byte, alignment: Byte, layoutScale: Float, surfaceWidth: Float, surfaceHeight: Float, pointerID: Int, x: Float, y: Float)
    
    // =========================================================================
    // Render Target Operations
    // =========================================================================
    
    external override fun cppCreateRenderTarget(pointer: Long, requestID: Long, width: Int, height: Int, sampleCount: Int)
    external override fun cppDeleteRenderTarget(pointer: Long, requestID: Long, renderTargetHandle: Long)
    external override fun cppCreateRiveRenderTarget(pointer: Long, width: Int, height: Int): Long
    external override fun cppCreateDrawKey(pointer: Long): Long
    
    // =========================================================================
    // Drawing Operations
    // =========================================================================
    
    external override fun cppDraw(pointer: Long, requestID: Long, artboardHandle: Long, smHandle: Long, surfacePtr: Long, renderTargetPtr: Long, drawKey: Long, surfaceWidth: Int, surfaceHeight: Int, fitMode: Int, alignmentMode: Int, clearColor: Int, scaleFactor: Float)
    
    external override fun cppDrawToBuffer(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, artboardHandle: Long, stateMachineHandle: Long, renderTargetPointer: Long, width: Int, height: Int, fit: Byte, alignment: Byte, scaleFactor: Float, clearColor: Int, buffer: ByteArray)
    
    external override fun cppRunOnCommandServer(pointer: Long, work: () -> Unit)
    
    // =========================================================================
    // Batch Sprite Rendering
    // =========================================================================
    
    external override fun cppDrawMultiple(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, renderTargetPointer: Long, viewportWidth: Int, viewportHeight: Int, clearColor: Int, artboardHandles: LongArray, stateMachineHandles: LongArray, transforms: FloatArray, artboardWidths: FloatArray, artboardHeights: FloatArray, count: Int)
    
    external override fun cppDrawMultipleToBuffer(pointer: Long, renderContextPointer: Long, surfaceNativePointer: Long, drawKey: Long, renderTargetPointer: Long, viewportWidth: Int, viewportHeight: Int, clearColor: Int, artboardHandles: LongArray, stateMachineHandles: LongArray, transforms: FloatArray, artboardWidths: FloatArray, artboardHeights: FloatArray, count: Int, buffer: ByteArray)
}

/**
 * Android implementation of [createCommandQueueBridge].
 * Returns a JNI-based bridge implementation.
 */
actual fun createCommandQueueBridge(): CommandQueueBridge = CommandQueueJNIBridge()

/**
 * Android implementation for closing native listeners.
 */
actual fun Listeners.closeNative() {
    cppDelete(
        fileListener,
        artboardListener,
        stateMachineListener,
        viewModelInstanceListener,
        imageListener,
        audioListener,
        fontListener
    )
}

/**
 * Native method to delete listeners.
 */
private external fun cppDelete(
    fileListener: Long,
    artboardListener: Long,
    stateMachineListener: Long,
    viewModelInstanceListener: Long,
    imageListener: Long,
    audioListener: Long,
    fontListener: Long
)
