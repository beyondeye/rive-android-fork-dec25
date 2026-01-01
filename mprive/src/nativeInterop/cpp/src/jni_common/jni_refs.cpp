#include "jni_refs.hpp"
#include <pthread.h>

namespace rive_mp {
    // Global JVM pointer - initialized in JNI_OnLoad
    JavaVM* g_JVM = nullptr;

    // Thread-local storage for class loader
    static jobject g_ClassLoader = nullptr;
    static jmethodID g_FindClassMethod = nullptr;

    void InitJNIClassLoader(JNIEnv* env, jobject contextObject) {
        // Find the class loader from the context
        // This is useful for loading classes dynamically in different classloader contexts
        if (contextObject != nullptr) {
            jclass contextClass = env->GetObjectClass(contextObject);
            jmethodID getClassLoaderMethod = env->GetMethodID(
                contextClass, 
                "getClass", 
                "()Ljava/lang/Class;"
            );
            
            if (getClassLoaderMethod != nullptr) {
                jobject classObject = env->CallObjectMethod(contextObject, getClassLoaderMethod);
                jclass classClass = env->FindClass("java/lang/Class");
                jmethodID getClassLoaderMethod2 = env->GetMethodID(
                    classClass, 
                    "getClassLoader", 
                    "()Ljava/lang/ClassLoader;"
                );
                
                if (getClassLoaderMethod2 != nullptr) {
                    jobject classLoader = env->CallObjectMethod(classObject, getClassLoaderMethod2);
                    if (classLoader != nullptr) {
                        g_ClassLoader = env->NewGlobalRef(classLoader);
                        
                        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
                        g_FindClassMethod = env->GetMethodID(
                            classLoaderClass,
                            "loadClass",
                            "(Ljava/lang/String;)Ljava/lang/Class;"
                        );
                    }
                }
            }
        }
    }

    JNIEnv* GetJNIEnv() {
        if (g_JVM == nullptr) {
            return nullptr;
        }

        JNIEnv* env = nullptr;
        jint result = g_JVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        
        if (result == JNI_EDETACHED) {
            // Thread is not attached, attach it
            result = g_JVM->AttachCurrentThread(&env, nullptr);
            if (result != JNI_OK) {
                return nullptr;
            }
        } else if (result != JNI_OK) {
            return nullptr;
        }
        
        return env;
    }
}
