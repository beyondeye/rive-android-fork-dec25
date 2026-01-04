package app.rive.mp.test.utils

/**
 * Platform-agnostic test context for initializing Rive.
 * Phase A: Minimal implementation for basic tests.
 */
expect class MpTestContext() {
    /**
     * Initialize Rive runtime for testing.
     * On Android, this calls Rive.init(context).
     * On Desktop, this may be a no-op or minimal setup.
     */
    fun initRive()
    
    /**
     * Get platform name for debugging.
     */
    fun getPlatformName(): String
    
    companion object {
        /**
         * Initialize the platform once for all tests.
         * This is called from test class init blocks to ensure Rive is initialized
         * before any tests run.
         */
        fun initPlatform()
    }
}
