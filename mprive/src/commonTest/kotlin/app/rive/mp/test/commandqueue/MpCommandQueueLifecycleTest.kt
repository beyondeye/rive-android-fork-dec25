package app.rive.mp.test.commandqueue

import app.rive.mp.CommandQueue
import app.rive.mp.test.utils.MpTestContext
import kotlin.test.*

/**
 * Phase A test for CommandQueue lifecycle management.
 * Tests reference counting, acquire/release, and disposal.
 */
class MpCommandQueueLifecycleTest {
    private val testContext = MpTestContext()
    
    @BeforeTest
    fun setup() {
        testContext.initRive()
    }
    
    @Test
    fun constructor_increments_refCount() {
        val queue = CommandQueue()
        assertEquals(1, queue.refCount, "CommandQueue should start with refCount = 1")
        assertFalse(queue.isDisposed, "CommandQueue should not be disposed initially")
        queue.release("test")
    }
    
    @Test
    fun acquire_increments_refCount() {
        val queue = CommandQueue()
        queue.acquire("test-owner")
        assertEquals(2, queue.refCount, "acquire() should increment refCount")
        queue.release("test-owner")
        queue.release("constructor")
    }
    
    @Test
    fun release_decrements_refCount() {
        val queue = CommandQueue()
        queue.acquire("test-owner")
        queue.release("test-owner")
        assertEquals(1, queue.refCount, "release() should decrement refCount")
        queue.release("constructor")
    }
    
    @Test
    fun release_to_zero_disposes() {
        val queue = CommandQueue()
        assertFalse(queue.isDisposed, "CommandQueue should not be disposed initially")
        assertFalse(queue.closed, "CommandQueue should not be closed initially")
        
        queue.release("constructor")
        
        assertTrue(queue.isDisposed, "CommandQueue should be disposed after final release")
        assertTrue(queue.closed, "CommandQueue should be closed after final release")
        assertEquals(0, queue.refCount, "refCount should be 0 after disposal")
    }
    
    @Test
    fun double_release_throws() {
        val queue = CommandQueue()
        queue.release("constructor")
        
        assertFailsWith<IllegalStateException>("Double release should throw") {
            queue.release("invalid")
        }
    }
    
    @Test
    fun acquire_after_disposal_throws() {
        val queue = CommandQueue()
        queue.release("constructor")
        
        assertFailsWith<IllegalStateException>("Acquire after disposal should throw") {
            queue.acquire("invalid")
        }
    }
    
    @Test
    fun close_releases_reference() {
        val queue = CommandQueue()
        queue.acquire("test-owner")
        
        // close() should call release()
        queue.close()
        
        assertEquals(1, queue.refCount, "close() should decrement refCount")
        
        // Clean up
        queue.release("constructor")
    }
}
