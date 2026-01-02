package app.rive.mp

/**
 * A handle to a Rive file on the CommandServer. Created with [CommandQueue.loadFile] and deleted
 * with [CommandQueue.deleteFile].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class FileHandle(val handle: Long) {
    override fun toString(): String = "FileHandle($handle)"
}

/**
 * A handle to an artboard instance on the CommandServer. Created with
 * [CommandQueue.createDefaultArtboard] or [CommandQueue.createArtboardByName]
 * and deleted with [CommandQueue.deleteArtboard].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class ArtboardHandle(val handle: Long) {
    override fun toString(): String = "ArtboardHandle($handle)"
}

/**
 * A handle to a state machine instance on the CommandServer. Created with
 * [CommandQueue.createDefaultStateMachine] or [CommandQueue.createStateMachineByName]
 * and deleted with [CommandQueue.deleteStateMachine].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class StateMachineHandle(val handle: Long) {
    override fun toString(): String = "StateMachineHandle($handle)"
}

/**
 * A handle to a view model instance on the CommandServer. Created with
 * [CommandQueue.createViewModelInstance] and deleted with [CommandQueue.deleteViewModelInstance].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class ViewModelInstanceHandle(val handle: Long) {
    override fun toString(): String = "ViewModelInstanceHandle($handle)"
}

/**
 * A handle to a RenderImage on the CommandServer. Created with [CommandQueue.decodeImage] and
 * deleted with [CommandQueue.deleteImage].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class ImageHandle(val handle: Long) {
    override fun toString(): String = "ImageHandle($handle)"
}

/**
 * A handle to an AudioSource on the CommandServer. Created with [CommandQueue.decodeAudio] and
 * deleted with [CommandQueue.deleteAudio].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class AudioHandle(val handle: Long) {
    override fun toString(): String = "AudioHandle($handle)"
}

/**
 * A handle to a Font on the CommandServer. Created with [CommandQueue.decodeFont] and deleted with
 * [CommandQueue.deleteFont].
 *
 * @param handle The handle issued by the native CommandQueue.
 */
@JvmInline
value class FontHandle(val handle: Long) {
    override fun toString(): String = "FontHandle($handle)"
}

/**
 * A key used to uniquely identify a draw operation in the CommandQueue. This is useful when the
 * same CommandQueue issues multiple draw calls. If the same key is used before the render loop
 * flushes the queue, the previous draw call will be replaced with the new one.
 *
 * @param handle The handle issued by the native CommandQueue for this draw operation.
 */
@JvmInline
value class DrawKey(val handle: Long) {
    override fun toString(): String = "DrawKey($handle)"
}
