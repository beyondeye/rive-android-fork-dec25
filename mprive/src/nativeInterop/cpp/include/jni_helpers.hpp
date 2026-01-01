#pragma once
#include <jni.h>
#include <string>
#include <vector>

namespace rive_mp {
    /**
     * Convert Java String to C++ std::string.
     * 
     * @param env JNI environment
     * @param jstr Java string
     * @return C++ string
     */
    std::string JStringToStdString(JNIEnv* env, jstring jstr);
    
    /**
     * Convert C++ std::string to Java String.
     * 
     * @param env JNI environment
     * @param str C++ string
     * @return Java string (local reference)
     */
    jstring StdStringToJString(JNIEnv* env, const std::string& str);
    
    /**
     * Convert Java byte array to C++ vector.
     * 
     * @param env JNI environment
     * @param arr Java byte array
     * @return C++ vector of bytes
     */
    std::vector<uint8_t> JByteArrayToVector(JNIEnv* env, jbyteArray arr);
    
    /**
     * Convert C++ vector to Java byte array.
     * 
     * @param env JNI environment
     * @param vec C++ vector of bytes
     * @return Java byte array (local reference)
     */
    jbyteArray VectorToJByteArray(JNIEnv* env, const std::vector<uint8_t>& vec);
    
    /**
     * Throw a RiveException from native code.
     * This will throw an exception that can be caught in Kotlin/Java code.
     * 
     * @param env JNI environment
     * @param message Error message
     */
    void ThrowRiveException(JNIEnv* env, const char* message);
    
    /**
     * Check if a JNI exception occurred and clear it.
     * Logs the exception if one occurred.
     * 
     * @param env JNI environment
     * @return true if an exception occurred, false otherwise
     */
    bool CheckAndClearException(JNIEnv* env);
}
