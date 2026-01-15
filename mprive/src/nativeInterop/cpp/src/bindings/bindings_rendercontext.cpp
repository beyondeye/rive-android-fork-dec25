/**
 * JNI bindings for RenderContextGL.
 * 
 * Phase C.2.2: Kotlin RenderContext Android Implementation
 * 
 * These bindings allow Kotlin to create and destroy C++ RenderContextGL objects.
 * The EGL display and context handles are passed from Kotlin to C++.
 */

#include <jni.h>
#include "render_context.hpp"
#include "rive_log.hpp"

// Log prefix for RenderContextJNI messages (embedded in log strings since macros already provide LOG_TAG)
#define RCJNI_PREFIX "[RenderContextJNI] "

extern "C" {

/**
 * Creates a new RenderContextGL from EGL display and context handles.
 * 
 * @param displayHandle The native handle of the EGLDisplay
 * @param contextHandle The native handle of the EGLContext
 * @return Pointer to the new RenderContextGL object
 */
JNIEXPORT jlong JNICALL
Java_app_rive_mp_RenderContextGL_cppConstructor(
    JNIEnv* env,
    jobject thiz,
    jlong displayHandle,
    jlong contextHandle
) {
    LOGD(RCJNI_PREFIX "Creating RenderContextGL from EGL handles");
    
    auto eglDisplay = reinterpret_cast<EGLDisplay>(displayHandle);
    auto eglContext = reinterpret_cast<EGLContext>(contextHandle);
    
    auto* renderContext = new rive_mp::RenderContextGL(eglDisplay, eglContext);
    
    LOGD(RCJNI_PREFIX "RenderContextGL created successfully");
    return reinterpret_cast<jlong>(renderContext);
}

/**
 * Deletes a RenderContextGL object.
 * 
 * @param pointer Pointer to the RenderContextGL object to delete
 */
JNIEXPORT void JNICALL
Java_app_rive_mp_RenderContextGL_cppDelete(
    JNIEnv* env,
    jobject thiz,
    jlong pointer
) {
    LOGD(RCJNI_PREFIX "Deleting RenderContextGL");
    
    auto* renderContext = reinterpret_cast<rive_mp::RenderContextGL*>(pointer);
    if (renderContext) {
        delete renderContext;
    }
    
    LOGD(RCJNI_PREFIX "RenderContextGL deleted");
}

} // extern "C"