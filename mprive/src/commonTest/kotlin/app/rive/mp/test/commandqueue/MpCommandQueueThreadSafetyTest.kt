package app.rive.mp.test.commandqueue

import app.rive.mp.CommandQueue
import app.rive.mp.test.utils.MpTestContext
import kotlinx.coroutines.*
import kotlinx.coroutines.test.runTest
import kotlin.test.*

/**
 * Phase A test for CommandQueue thread safety.
 * Tests concurrent acquire/release operations.
 */
class MpCommandQueueThreadSafetyTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun concurrent_acquire_release_is_safe() = runTest {
        val queue = CommandQueue()
        val iterations = 100 // Reduced from 1000 for faster tests
        val threads = 10
        
        // Launch multiple coroutines that concurrently acquire and release
        val jobs = (1..threads).map { threadId ->
            launch(Dispatchers.Default) {
                repeat(iterations) {
                    queue.acquire("thread-$threadId")
                    yield() // Encourage interleaving
                    queue.release("thread-$threadId")
                }
            }
        }
        
        jobs.joinAll()
        
        // Should end up back at refCount = 1 (constructor)
        assertEquals(1, queue.refCount, "refCount should be 1 after all concurrent operations")
        assertFalse(queue.isDisposed, "CommandQueue should not be disposed")
        
        // Clean up
        queue.release("constructor")
    }
    
    @Test
    fun concurrent_multiple_acquires_then_releases() = runTest {
        val queue = CommandQueue()
        val acquireCount = 50
        
        // Phase 1: Concurrent acquires
        val acquireJobs = (1..acquireCount).map { id ->
            launch(Dispatchers.Default) {
                queue.acquire("concurrent-$id")
            }
        }
        acquireJobs.joinAll()
        
        assertEquals(
            1 + acquireCount,
            queue.refCount,
            "refCount should be 1 + acquireCount after all acquires"
        )
        
        // Phase 2: Concurrent releases
        val releaseJobs = (1..acquireCount).map { id ->
            launch(Dispatchers.Default) {
                queue.release("concurrent-$id")
            }
        }
        releaseJobs.joinAll()
        
        assertEquals(1, queue.refCount, "refCount should be back to 1 after all releases")
        
        // Clean up
        queue.release("constructor")
    }
    
    @Test
    fun refCount_never_goes_negative() = runTest {
        val queue = CommandQueue()
        var minRefCount = Int.MAX_VALUE
        val iterations = 100
        
        // Monitor refCount while doing concurrent operations
        val monitorJob = launch(Dispatchers.Default) {
            repeat(iterations * 20) {
                val current = queue.refCount
                if (current < minRefCount) {
                    minRefCount = current
                }
                yield()
            }
        }
        
        // Concurrent acquire/release
        val jobs = (1..10).map { id ->
            launch(Dispatchers.Default) {
                repeat(iterations) {
                    queue.acquire("thread-$id")
                    yield()
                    queue.release("thread-$id")
                }
            }
        }
        
        jobs.joinAll()
        monitorJob.cancel()
        
        assertTrue(minRefCount >= 1, "refCount should never go below 1 during concurrent operations")
        
        // Clean up
        queue.release("constructor")
    }
}
