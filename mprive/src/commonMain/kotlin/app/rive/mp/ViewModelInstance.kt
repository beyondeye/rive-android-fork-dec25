/**
 * ViewModelInstance - Multiplatform Rive view model instance wrapper
 *
 * A view model instance for data binding with Rive animations.
 *
 * Usage:
 * ```kotlin
 * // Using rememberViewModelInstance composable (recommended):
 * val vmiResult = rememberViewModelInstance(file, "MyViewModel")
 *
 * // Or with a specific instance name:
 * val vmiResult = rememberViewModelInstance(file, "MyViewModel", "instance1")
 *
 * // Using ViewModelInstance.fromFile directly (manual lifecycle management):
 * val vmi = ViewModelInstance.fromFile(file, "MyViewModel")
 * try {
 *     vmi.setStringProperty("title", "Hello World")
 *     // Use the VMI...
 * } finally {
 *     vmi.close()
 * }
 * ```
 */
package app.rive.mp

import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow

private const val VMI_TAG = "Rive/VMI"

/**
 * A view model instance for data binding with Rive animations.
 *
 * ViewModelInstances allow you to bind data to Rive animations. Properties on the VMI
 * can be get/set, and changes will be reflected in the animation when bound to a state machine.
 *
 * Create an instance using [rememberViewModelInstance] composable or the static factory methods.
 * When using the factory methods, ensure you call [close] when done to release resources.
 *
 * @param instanceHandle The handle to the VMI on the command server.
 * @param riveWorker The CommandQueue that owns the VMI.
 * @param fileHandle The file handle that owns this VMI.
 * @param viewModelName The name of the ViewModel this instance is based on.
 * @param instanceName The name of the instance, or null if blank/default.
 */
class ViewModelInstance internal constructor(
    val instanceHandle: ViewModelInstanceHandle,
    internal val riveWorker: CommandQueue,
    internal val fileHandle: FileHandle,
    val viewModelName: String,
    val instanceName: String?,
) : AutoCloseable {

    private var closed = false

    /**
     * A flow that emits Unit whenever a property on this VMI is changed.
     * Subscribe to this to know when to re-render.
     */
    private val _dirtyFlow = MutableSharedFlow<Unit>(
        replay = 0,
        extraBufferCapacity = 1,
        onBufferOverflow = kotlinx.coroutines.channels.BufferOverflow.DROP_OLDEST
    )
    val dirtyFlow: SharedFlow<Unit> = _dirtyFlow

    companion object {
        /**
         * Creates a blank [ViewModelInstance] from a named ViewModel.
         * A blank instance has all properties set to their default values.
         *
         * ⚠️ The lifetime of the returned VMI is managed by the caller. Make sure to call
         * [close] when you are done with it to release its resources.
         *
         * @param file The [RiveFile] containing the ViewModel.
         * @param viewModelName The name of the ViewModel to create an instance of.
         * @return The created ViewModelInstance.
         * @throws IllegalArgumentException If the VMI cannot be created.
         */
        fun createBlank(
            file: RiveFile,
            viewModelName: String
        ): ViewModelInstance {
            val handle = file.riveWorker.createBlankViewModelInstance(file.fileHandle, viewModelName)
            RiveLog.d(VMI_TAG) { "Created blank VMI $handle for '$viewModelName'" }
            return ViewModelInstance(handle, file.riveWorker, file.fileHandle, viewModelName, null)
        }

        /**
         * Creates a default [ViewModelInstance] from a named ViewModel.
         * A default instance uses the ViewModel's default instance values if available.
         *
         * ⚠️ The lifetime of the returned VMI is managed by the caller. Make sure to call
         * [close] when you are done with it to release its resources.
         *
         * @param file The [RiveFile] containing the ViewModel.
         * @param viewModelName The name of the ViewModel to create an instance of.
         * @return The created ViewModelInstance.
         * @throws IllegalArgumentException If the VMI cannot be created.
         */
        fun createDefault(
            file: RiveFile,
            viewModelName: String
        ): ViewModelInstance {
            val handle = file.riveWorker.createDefaultViewModelInstance(file.fileHandle, viewModelName)
            RiveLog.d(VMI_TAG) { "Created default VMI $handle for '$viewModelName'" }
            return ViewModelInstance(handle, file.riveWorker, file.fileHandle, viewModelName, null)
        }

        /**
         * Creates a named [ViewModelInstance] from a named ViewModel.
         * Uses the values defined for the named instance in the Rive file.
         *
         * ⚠️ The lifetime of the returned VMI is managed by the caller. Make sure to call
         * [close] when you are done with it to release its resources.
         *
         * @param file The [RiveFile] containing the ViewModel.
         * @param viewModelName The name of the ViewModel to create an instance of.
         * @param instanceName The name of the specific instance to create.
         * @return The created ViewModelInstance.
         * @throws IllegalArgumentException If the VMI cannot be created.
         */
        fun createNamed(
            file: RiveFile,
            viewModelName: String,
            instanceName: String
        ): ViewModelInstance {
            val handle = file.riveWorker.createNamedViewModelInstance(
                file.fileHandle, viewModelName, instanceName
            )
            RiveLog.d(VMI_TAG) { "Created named VMI $handle for '$viewModelName' instance '$instanceName'" }
            return ViewModelInstance(handle, file.riveWorker, file.fileHandle, viewModelName, instanceName)
        }
    }

    /**
     * Closes this VMI and releases its resources.
     *
     * After calling this method, the ViewModelInstance instance should not be used.
     * This method is idempotent - it's safe to call multiple times.
     */
    override fun close() {
        if (closed) return
        closed = true

        val nameLog = instanceName?.let { "instance '$it'" } ?: "(default/blank)"
        RiveLog.d(VMI_TAG) { "Deleting $instanceHandle '$viewModelName' $nameLog" }
        riveWorker.deleteViewModelInstance(instanceHandle)
    }

    /**
     * Marks this VMI as dirty, triggering a re-render.
     */
    internal fun markDirty() {
        _dirtyFlow.tryEmit(Unit)
    }

    // =============================================================================
    // Property Operations - Number
    // =============================================================================

    /**
     * Gets a number property value.
     *
     * @param propertyPath The path to the property.
     * @return The current value of the property.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getNumberProperty(propertyPath: String): Float {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getNumberProperty(instanceHandle, propertyPath)
    }

    /**
     * Sets a number property value.
     *
     * @param propertyPath The path to the property.
     * @param value The value to set.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setNumberProperty(propertyPath: String, value: Float) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.setNumberProperty(instanceHandle, propertyPath, value)
        markDirty()
    }

    // =============================================================================
    // Property Operations - String
    // =============================================================================

    /**
     * Gets a string property value.
     *
     * @param propertyPath The path to the property.
     * @return The current value of the property.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getStringProperty(propertyPath: String): String {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getStringProperty(instanceHandle, propertyPath)
    }

    /**
     * Sets a string property value.
     *
     * @param propertyPath The path to the property.
     * @param value The value to set.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setStringProperty(propertyPath: String, value: String) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.setStringProperty(instanceHandle, propertyPath, value)
        markDirty()
    }

    // =============================================================================
    // Property Operations - Boolean
    // =============================================================================

    /**
     * Gets a boolean property value.
     *
     * @param propertyPath The path to the property.
     * @return The current value of the property.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getBooleanProperty(propertyPath: String): Boolean {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getBooleanProperty(instanceHandle, propertyPath)
    }

    /**
     * Sets a boolean property value.
     *
     * @param propertyPath The path to the property.
     * @param value The value to set.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setBooleanProperty(propertyPath: String, value: Boolean) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.setBooleanProperty(instanceHandle, propertyPath, value)
        markDirty()
    }

    // =============================================================================
    // Property Operations - Enum
    // =============================================================================

    /**
     * Gets an enum property value.
     *
     * @param propertyPath The path to the property.
     * @return The current enum value as a string.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getEnumProperty(propertyPath: String): String {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getEnumProperty(instanceHandle, propertyPath)
    }

    /**
     * Sets an enum property value.
     *
     * @param propertyPath The path to the property.
     * @param value The enum value to set (as a string).
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setEnumProperty(propertyPath: String, value: String) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.setEnumProperty(instanceHandle, propertyPath, value)
        markDirty()
    }

    // =============================================================================
    // Property Operations - Color
    // =============================================================================

    /**
     * Gets a color property value.
     *
     * @param propertyPath The path to the property.
     * @return The current color value in 0xAARRGGBB format.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getColorProperty(propertyPath: String): Int {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getColorProperty(instanceHandle, propertyPath)
    }

    /**
     * Sets a color property value.
     *
     * @param propertyPath The path to the property.
     * @param value The color value in 0xAARRGGBB format.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun setColorProperty(propertyPath: String, value: Int) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.setColorProperty(instanceHandle, propertyPath, value)
        markDirty()
    }

    // =============================================================================
    // Property Operations - Trigger
    // =============================================================================

    /**
     * Fires a trigger property.
     *
     * @param propertyPath The path to the trigger property.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun fireTriggerProperty(propertyPath: String) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.fireTriggerProperty(instanceHandle, propertyPath)
        markDirty()
    }

    // =============================================================================
    // List Operations
    // =============================================================================

    /**
     * Gets the size of a list property.
     *
     * @param propertyPath The path to the list property.
     * @return The number of items in the list.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getListSize(propertyPath: String): Int {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getListSize(instanceHandle, propertyPath)
    }

    /**
     * Gets an item from a list property by index.
     *
     * @param propertyPath The path to the list property.
     * @param index The index of the item to get (0-based).
     * @return A handle to the ViewModelInstance at the specified index.
     * @throws IllegalStateException If the VMI has been closed.
     */
    suspend fun getListItem(propertyPath: String, index: Int): ViewModelInstanceHandle {
        check(!closed) { "ViewModelInstance has been closed" }
        return riveWorker.getListItem(instanceHandle, propertyPath, index)
    }

    /**
     * Adds an item to the end of a list property.
     *
     * @param propertyPath The path to the list property.
     * @param itemHandle The handle of the VMI to add.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun addListItem(propertyPath: String, itemHandle: ViewModelInstanceHandle) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.addListItem(instanceHandle, propertyPath, itemHandle)
        markDirty()
    }

    /**
     * Removes an item from a list property by index.
     *
     * @param propertyPath The path to the list property.
     * @param index The index of the item to remove.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun removeListItemAt(propertyPath: String, index: Int) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.removeListItemAt(instanceHandle, propertyPath, index)
        markDirty()
    }

    // =============================================================================
    // Property Subscriptions
    // =============================================================================

    /**
     * Subscribe to changes to a property.
     *
     * @param propertyPath The path to the property.
     * @param propertyType The data type of the property.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun subscribeToProperty(propertyPath: String, propertyType: PropertyDataType) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.subscribeToProperty(instanceHandle, propertyPath, propertyType)
    }

    /**
     * Unsubscribe from changes to a property.
     *
     * @param propertyPath The path to the property.
     * @param propertyType The data type of the property.
     * @throws IllegalStateException If the VMI has been closed.
     */
    @Throws(IllegalStateException::class)
    fun unsubscribeFromProperty(propertyPath: String, propertyType: PropertyDataType) {
        check(!closed) { "ViewModelInstance has been closed" }
        riveWorker.unsubscribeFromProperty(instanceHandle, propertyPath, propertyType)
    }

    override fun toString(): String = "ViewModelInstance($instanceHandle, viewModel=$viewModelName, instance=$instanceName)"
}