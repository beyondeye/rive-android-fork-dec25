#pragma once
#include <jni.h>

namespace rive_mp {
    // Global JVM pointer for accessing JNI environment from any thread
    extern JavaVM* g_JVM;
    
    /**
     * Initialize JNI class loader for dynamic class loading.
     * This should be called once during JNI initialization.
     * 
     * @param env JNI environment
     * @param contextObject Context object (can be null)
     */
    void InitJNIClassLoader(JNIEnv* env, jobject contextObject);
    
    /**
     * Get JNIEnv for the current thread.
     * This handles thread attachment if the thread is not already attached to the JVM.
     * 
     * @return JNIEnv pointer for current thread, or nullptr if JVM is not initialized
     */
    JNIEnv* GetJNIEnv();
}
