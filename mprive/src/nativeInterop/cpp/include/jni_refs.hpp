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
    
    /**
     * RAII wrapper for JNI global references.
     * Automatically creates and deletes global references.
     */
    template<typename T>
    class GlobalRef {
    public:
        GlobalRef() : m_ref(nullptr) {}
        
        GlobalRef(JNIEnv* env, T localRef) {
            if (localRef) {
                m_ref = static_cast<T>(env->NewGlobalRef(localRef));
            } else {
                m_ref = nullptr;
            }
        }
        
        ~GlobalRef() {
            if (m_ref) {
                JNIEnv* env = GetJNIEnv();
                if (env) {
                    env->DeleteGlobalRef(m_ref);
                }
            }
        }
        
        // Non-copyable
        GlobalRef(const GlobalRef&) = delete;
        GlobalRef& operator=(const GlobalRef&) = delete;
        
        // Movable
        GlobalRef(GlobalRef&& other) noexcept : m_ref(other.m_ref) {
            other.m_ref = nullptr;
        }
        
        GlobalRef& operator=(GlobalRef&& other) noexcept {
            if (this != &other) {
                if (m_ref) {
                    JNIEnv* env = GetJNIEnv();
                    if (env) {
                        env->DeleteGlobalRef(m_ref);
                    }
                }
                m_ref = other.m_ref;
                other.m_ref = nullptr;
            }
            return *this;
        }
        
        T get() const { return m_ref; }
        operator T() const { return m_ref; }
        explicit operator bool() const { return m_ref != nullptr; }
        
    private:
        T m_ref;
    };
}
