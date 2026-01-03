package app.rive.mp

/**
 * Desktop-specific stub implementation of RenderContext.
 * Phase A: Minimal stub for compilation. Full implementation in Phase C/F.
 */
internal class RenderContextDesktopStub : RenderContext() {
    override val nativeObjectPointer: Long = 0L
    
    override fun createSurface(drawKey: DrawKey, commandQueue: CommandQueue): RiveSurface {
        TODO("Phase C: Desktop surface creation not yet implemented")
    }
    
    override fun createImageSurface(
        width: Int,
        height: Int,
        drawKey: DrawKey,
        commandQueue: CommandQueue
    ): RiveSurface {
        TODO("Phase C: Desktop image surface creation not yet implemented")
    }
    
    override fun close() {
        // Stub - nothing to clean up yet
    }
    
    override val closed: Boolean = false
}

/**
 * Desktop-specific actual implementation of createDefaultRenderContext.
 * Phase A: Returns a stub. Full implementation in Phase C/F.
 */
actual fun createDefaultRenderContext(): RenderContext {
    RiveLog.d("Rive/RenderContext") { "Creating stub Desktop RenderContext (Phase A)" }
    return RenderContextDesktopStub()
}
