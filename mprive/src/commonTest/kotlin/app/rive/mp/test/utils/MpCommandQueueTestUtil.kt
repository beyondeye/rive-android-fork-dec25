package app.rive.mp.test.utils

import app.rive.mp.CommandQueue
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * Test utility for managing CommandQueue instances with automatic message polling.
 * 
 * This utility creates a CommandQueue and starts a coroutine that polls messages
 * at regular intervals. This is necessary for the CommandQueue to deliver callbacks
 * and resume suspended coroutines.
 * 
 * Usage:
 * ```
 * runTest {
 *     val testUtil = MpCommandQueueTestUtil(this)
 *     val commandQueue = testUtil.commandQueue
 *     
 *     // Use commandQueue for testing
 *     val fileHandle = commandQueue.loadFile(bytes)
 *     
 *     // Cleanup automatically handled by testUtil
 *     testUtil.cleanup()
 * }
 * ```
 */
class MpCommandQueueTestUtil(
    private val testScope: CoroutineScope,
    private val pollIntervalMs: Long = 16L  // ~60 FPS
) {
    /**
     * The CommandQueue instance managed by this utility.
     */
    val commandQueue: CommandQueue = CommandQueue()
    
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
     * Cleanup the CommandQueue and stop polling.
     * Should be called at the end of each test.
     */
    fun cleanup() {
        pollingJob.cancel()
        commandQueue.release("test", "cleanup")
    }
}

/**
 * Helper function to create a CommandQueue with automatic polling for tests.
 * 
 * @param testScope The test coroutine scope.
 * @param pollIntervalMs The polling interval in milliseconds (default: 16ms ~60 FPS).
 * @param block The test block to execute with the CommandQueue.
 */
suspend fun withCommandQueue(
    testScope: CoroutineScope,
    pollIntervalMs: Long = 16L,
    block: suspend (CommandQueue) -> Unit
) {
    val testUtil = MpCommandQueueTestUtil(testScope, pollIntervalMs)
    try {
        block(testUtil.commandQueue)
    } finally {
        testUtil.cleanup()
    }
}
