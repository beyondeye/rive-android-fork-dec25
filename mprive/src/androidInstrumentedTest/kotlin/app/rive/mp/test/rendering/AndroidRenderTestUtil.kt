package app.rive.mp.test.rendering

import app.rive.mp.CommandQueue
import app.rive.mp.DrawKey
import app.rive.mp.RenderContext
import app.rive.mp.RiveSurface
import app.rive.mp.createDefaultRenderContext
import app.rive.mp.test.utils.MpTestContext
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * Test utility for Android rendering tests.
 * 
 * Provides PBuffer surface creation and CommandQueue management for E2E rendering tests.
 * Uses EGL PBuffer surfaces which don't require a visible UI, making them suitable for
 * instrumented tests.
 * 
 * Usage:
 * ```kotlin
 * runTest {
 *     val testUtil = AndroidRenderTestUtil(this)
 *     try {
 *         val surface = testUtil.createTestSurface(100, 100)
 *         // ... use surface for rendering ...
 *         surface.close()
 *     } finally {
 *         testUtil.cleanup()
 *     }
 * }
 * ```
 */
class AndroidRenderTestUtil(
    private val testScope: CoroutineScope,
    private val pollIntervalMs: Long = 16L  // ~60 FPS
) {
    
    init {
        // Ensure Rive is initialized for Android tests
        MpTestContext.initPlatform()
    }
    
    /**
     * The RenderContext used for creating surfaces.
     * This is an EGL-based context on Android.
     */
    val renderContext: RenderContext = createDefaultRenderContext()
    
    /**
     * The CommandQueue instance managed by this utility.
     * Configured with the renderContext for surface operations.
     */
    val commandQueue: CommandQueue = CommandQueue(renderContext)
    
    /**
     * The polling job that calls pollMessages() periodically.
     */
    private val pollingJob: Job
    
    init {
        // Start automatic polling
        pollingJob = testScope.launch {
            while (isActive) {
                try {
                    commandQueue.pollMessages()
                } catch (e: Exception) {
                    // Ignore errors during cleanup
                    if (isActive) {
                        throw e
                    }
                }
                delay(pollIntervalMs)
            }
        }
    }
    
    /**
     * Create a PBuffer surface for testing.
     * 
     * PBuffer surfaces are off-screen EGL surfaces that can be used for rendering
     * without requiring a visible UI. This is ideal for instrumented tests.
     * 
     * @param width The width of the surface in pixels.
     * @param height The height of the surface in pixels.
     * @return A [RiveSurface] ready for rendering.
     */
    fun createTestSurface(width: Int, height: Int): RiveSurface {
        val drawKey = commandQueue.createDrawKey()
        return renderContext.createImageSurface(width, height, drawKey, commandQueue)
    }
    
    /**
     * Generate a unique DrawKey for testing.
     * 
     * @return A unique [DrawKey].
     */
    fun createDrawKey(): DrawKey {
        return commandQueue.createDrawKey()
    }
    
    /**
     * Cleanup the CommandQueue, RenderContext, and stop polling.
     * Should be called at the end of each test.
     */
    fun cleanup() {
        pollingJob.cancel()
        commandQueue.release("test", "cleanup")
    }
}

/**
 * Helper function to run a rendering test with automatic resource management.
 * 
 * @param testScope The test coroutine scope.
 * @param block The test block to execute with the render test utility.
 */
suspend fun withRenderTestUtil(
    testScope: CoroutineScope,
    block: suspend (AndroidRenderTestUtil) -> Unit
) {
    val testUtil = AndroidRenderTestUtil(testScope)
    try {
        block(testUtil)
    } finally {
        testUtil.cleanup()
    }
}
