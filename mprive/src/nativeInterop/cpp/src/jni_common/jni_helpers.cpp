#include "jni_helpers.hpp"
#include <cstring>

namespace rive_mp {
    std::string JStringToStdString(JNIEnv* env, jstring jstr) {
        if (jstr == nullptr) {
            return "";
        }
        
        const char* chars = env->GetStringUTFChars(jstr, nullptr);
        if (chars == nullptr) {
            return "";
        }
        
        std::string result(chars);
        env->ReleaseStringUTFChars(jstr, chars);
        
        return result;
    }
    
    jstring StdStringToJString(JNIEnv* env, const std::string& str) {
        return env->NewStringUTF(str.c_str());
    }
    
    std::vector<uint8_t> JByteArrayToVector(JNIEnv* env, jbyteArray arr) {
        if (arr == nullptr) {
            return std::vector<uint8_t>();
        }
        
        jsize length = env->GetArrayLength(arr);
        std::vector<uint8_t> result(length);
        
        if (length > 0) {
            jbyte* bytes = env->GetByteArrayElements(arr, nullptr);
            if (bytes != nullptr) {
                std::memcpy(result.data(), bytes, length);
                env->ReleaseByteArrayElements(arr, bytes, JNI_ABORT);
            }
        }
        
        return result;
    }
    
    jbyteArray VectorToJByteArray(JNIEnv* env, const std::vector<uint8_t>& vec) {
        jsize length = static_cast<jsize>(vec.size());
        jbyteArray result = env->NewByteArray(length);
        
        if (result != nullptr && length > 0) {
            env->SetByteArrayRegion(
                result, 
                0, 
                length, 
                reinterpret_cast<const jbyte*>(vec.data())
            );
        }
        
        return result;
    }
    
    void ThrowRiveException(JNIEnv* env, const char* message) {
        // Try to find the RiveException class
        // First try app.rive.mp package (multiplatform)
        jclass exceptionClass = env->FindClass("app/rive/mp/RiveException");
        
        if (exceptionClass == nullptr) {
            // Clear the exception and try app.rive.runtime.kotlin package (original kotlin module)
            env->ExceptionClear();
            exceptionClass = env->FindClass("app/rive/runtime/kotlin/core/RiveException");
        }
        
        if (exceptionClass == nullptr) {
            // Fallback to standard RuntimeException
            env->ExceptionClear();
            exceptionClass = env->FindClass("java/lang/RuntimeException");
        }
        
        if (exceptionClass != nullptr) {
            env->ThrowNew(exceptionClass, message);
        }
    }
    
    bool CheckAndClearException(JNIEnv* env) {
        if (env->ExceptionCheck()) {
            // Exception occurred
            jthrowable exception = env->ExceptionOccurred();
            env->ExceptionClear();
            
            // Log the exception (optional - could use rive_log.hpp when available)
            if (exception != nullptr) {
                jclass throwableClass = env->FindClass("java/lang/Throwable");
                if (throwableClass != nullptr) {
                    jmethodID getMessageMethod = env->GetMethodID(
                        throwableClass, 
                        "getMessage", 
                        "()Ljava/lang/String;"
                    );
                    if (getMessageMethod != nullptr) {
                        jstring messageStr = static_cast<jstring>(
                            env->CallObjectMethod(exception, getMessageMethod)
                        );
                        if (messageStr != nullptr) {
                            // Could log this message if logging is available
                            // For now, just ensure cleanup
                            env->DeleteLocalRef(messageStr);
                        }
                    }
                }
                env->DeleteLocalRef(exception);
            }
            
            return true;
        }
        return false;
    }
}
