cmake_minimum_required(VERSION 3.18.1)

project(mprive-android VERSION 1.0.0 LANGUAGES CXX)

# Compile detail will be in rive-android/mprive/.cxx/Debug/<hash>/<ABI>/compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_VERBOSE_MAKEFILE ON)

# =============================================================================
# Source File Discovery (Using GLOB for automatic inclusion of new files)
# =============================================================================
# 
# NOTE: When adding new .cpp files to existing directories, they will be
# automatically included. If GLOB doesn't detect changes, run a clean build:
#   ./gradlew clean
#   ./gradlew :mprive:build
#
# Or delete the CMake cache:
#   rm -rf mprive/.cxx/
#   ./gradlew :mprive:build

# Find shared native interop sources (platform-agnostic code)
# NOTE: Temporarily excluding render_buffer.cpp - it needs Rive Alignment API fixes
file(GLOB NATIVE_INTEROP_JNI_COMMON CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../nativeInterop/cpp/src/jni_common/*.cpp"
)
list(REMOVE_ITEM NATIVE_INTEROP_JNI_COMMON
        "${CMAKE_CURRENT_SOURCE_DIR}/../../nativeInterop/cpp/src/jni_common/render_buffer.cpp"
)
file(GLOB NATIVE_INTEROP_BINDINGS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../nativeInterop/cpp/src/bindings/*.cpp"
)
file(GLOB NATIVE_INTEROP_COMMAND_SERVER CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../nativeInterop/cpp/src/command_server/*.cpp"
)

# Find Android-specific sources
file(GLOB ANDROID_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Find Android-specific helper sources (image decoding, platform factory)
file(GLOB ANDROID_HELPERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/helpers/*.cpp"
)

# Rive utility sources from rive-runtime/utils/
#
# WHY: The rive-runtime static library (librive.a) built by Premake does NOT include
# the utils/ directory sources. These are optional utilities meant to be compiled
# separately when needed.
#
# WHAT: NoOpFactory is a lightweight Factory implementation that returns null/empty
# render objects. It allows loading and manipulating Rive files without a real
# graphics context (OpenGL/Vulkan).
#
# WHEN NEEDED: The CommandServer uses NoOpFactory as a fallback when no RenderContext
# is provided (i.e., when renderContextPtr == 0). This enables:
#   1. Unit tests that don't need rendering
#   2. Headless file manipulation (loading files, querying artboards, view models)
#   3. Server-side Rive file processing
#
# Without this, File::import() would crash with "assertion 'factory' failed" because
# rive-runtime requires a non-null Factory* to load files.
set(RIVE_UTILS_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../submodules/rive-runtime/utils/no_op_factory.cpp"
)

# Combine all sources
set(ALL_SOURCES
        ${NATIVE_INTEROP_JNI_COMMON}
        ${NATIVE_INTEROP_BINDINGS}
        ${NATIVE_INTEROP_COMMAND_SERVER}
        ${ANDROID_SOURCES}
        ${ANDROID_HELPERS}
        ${RIVE_UTILS_SOURCES}
)

# Debug: Print discovered files (visible during build)
message(STATUS "Native Interop JNI Common files: ${NATIVE_INTEROP_JNI_COMMON}")
message(STATUS "Native Interop Bindings files: ${NATIVE_INTEROP_BINDINGS}")
message(STATUS "Native Interop Command Server files: ${NATIVE_INTEROP_COMMAND_SERVER}")
message(STATUS "Android-specific files: ${ANDROID_SOURCES}")
message(STATUS "Android helper files: ${ANDROID_HELPERS}")
message(STATUS "Total source files: ${ALL_SOURCES}")

# Create the mprive-android library target
add_library(mprive-android SHARED ${ALL_SOURCES})

# =============================================================================
# Compiler Configuration
# =============================================================================

# Enable C++17
target_compile_features(mprive-android PUBLIC cxx_std_17)

# Compiler flags
target_compile_options(mprive-android PRIVATE
        -Wall # Enable all warnings
        -fno-exceptions # Disable C++ exceptions
        -fno-rtti # Disable C++ RTTI
        $<$<CONFIG:Release>:-Oz>) # Optimize for size with -Oz in release

# Compile definitions
target_compile_definitions(mprive-android PRIVATE
        RIVE_ANDROID=
        YOGA_EXPORT=
        $<$<CONFIG:Debug>:DEBUG>)

# =============================================================================
# Rive Runtime Configuration
# =============================================================================

# Configure for monorepo vs. rive-android repo
if (EXISTS "${PROJECT_SOURCE_DIR}/../../../../submodules/rive-runtime")
    set(PACKAGES_DIR "${PROJECT_SOURCE_DIR}/../../../../")
    set(RIVE_RUNTIME_DIR "${PACKAGES_DIR}/submodules/rive-runtime")
else ()
    set(PACKAGES_DIR "${PROJECT_SOURCE_DIR}/../../../../..")
    set(RIVE_RUNTIME_DIR "${PACKAGES_DIR}/runtime")
endif ()

# Pass the resolved runtime directory to the environment for Premake
set(ENV{RIVE_RUNTIME_DIR} "${RIVE_RUNTIME_DIR}")

message(STATUS "Rive Runtime Directory: ${RIVE_RUNTIME_DIR}")

# =============================================================================
# Optional Features
# =============================================================================

# Add an option to remove miniaudio
# Passed from Gradle with -DWITH_RIVE_AUDIO=ON|OFF
option(WITH_RIVE_AUDIO "Enable Rive audio support" ON)
if (WITH_RIVE_AUDIO)
    # Compile flag picked up by JNI C++
    target_compile_definitions(mprive-android PRIVATE
            WITH_RIVE_AUDIO
            MA_NO_RESOURCE_MANAGER)
    # Value for Premake --with_rive_audio
    set(RIVE_AUDIO_ARG "system")
    # Add to the list of static libraries
    set(static_libs_with_audio miniaudio)
else ()
    # Value for Premake --with_rive_audio
    set(RIVE_AUDIO_ARG "disabled")
    set(static_libs_with_audio "")
endif ()

# Add an option to enable ASAN
# Passed from Gradle with -DENABLE_ASAN=ON|OFF
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
if (ENABLE_ASAN)
    # Pass to Premake to enable ASAN on librive and its dependencies
    set(RIVE_ASAN_FLAG "--with-asan")

    # Add ASAN flags to mprive-android as well
    target_compile_options(mprive-android PRIVATE
            -fsanitize=address
            -fno-omit-frame-pointer)
    target_link_options(mprive-android PRIVATE
            -fsanitize=address
            -Wl,--export-dynamic
            -Wl,--no-undefined)
endif ()

# =============================================================================
# Build Configuration
# =============================================================================

# Debug/release configuration
message(STATUS "libmprive-android build type: ${CMAKE_BUILD_TYPE}")
#[[ CMake adds these flags depending on CMAKE_BUILD_TYPE
    1. Debug: `-O0 -g`
    2. Release: `-O3 -DNDEBUG`
    3. RelWithDebInfo: `-O2 -g -DNDEBUG`
    4. MinSizeRel: `-Os -DNDEBUG`
]]
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CONFIG "debug")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release" OR
        CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR
        CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CONFIG "release")
endif ()

# Translate Android ABI to Rive architecture
if (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(RIVE_ARCH "arm64")
elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(RIVE_ARCH "arm")
elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(RIVE_ARCH "x64")
elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(RIVE_ARCH "x86")
else ()
    message(FATAL_ERROR "Unsupported ABI: ${CMAKE_ANDROID_ARCH_ABI}")
endif ()

message(STATUS "Target ABI: ${CMAKE_ANDROID_ARCH_ABI} -> Rive arch: ${RIVE_ARCH}")

# =============================================================================
# Build Rive C++ Runtime
# =============================================================================

# Promote our ${ANDROID_NDK} to an environment var so premake uses the same one.
set(ENV{ANDROID_NDK} "${ANDROID_NDK}")

# Build the Rive C++ runtime and its dependencies as static .a libs
execute_process(
        COMMAND bash ${RIVE_RUNTIME_DIR}/build/build_rive.sh --file=premake5_cpp_runtime.lua android ninja ${CONFIG} ${RIVE_ARCH} --no-rive-decoders --with_rive_audio=${RIVE_AUDIO_ARG} ${RIVE_ASAN_FLAG} -- rive_cpp_runtime
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE SCRIPT_RESULT
        OUTPUT_VARIABLE SCRIPT_OUTPUT
        ECHO_OUTPUT_VARIABLE
)
if (NOT SCRIPT_RESULT EQUAL "0")
    message(FATAL_ERROR "Premake script returned with error: '${SCRIPT_OUTPUT}' - '${SCRIPT_RESULT}'")
endif ()

# =============================================================================
# Include Directories
# =============================================================================

# Add header files from Rive runtime
# NOTE: ${RIVE_RUNTIME_DIR} (without /include) is added to support #include "utils/no_op_factory.hpp"
# which is how the header is referenced in command_server.hpp. The utils/ directory lives at
# the root of rive-runtime, not under include/.
#
# Android-specific helpers directory is added for platform-native image decoding support.
target_include_directories(mprive-android PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../nativeInterop/cpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RIVE_RUNTIME_DIR}/include
        ${RIVE_RUNTIME_DIR}/renderer/include
        ${RIVE_RUNTIME_DIR})

# =============================================================================
# Link Static Libraries
# =============================================================================

# List of static libraries to link
set(static_libs
        rive
        rive_harfbuzz
        rive_sheenbidi
        rive_yoga
        rive_pls_renderer
        ${static_libs_with_audio}
)

# Link each static library
foreach (X IN LISTS static_libs)
    set(LIB_TARGET "${X}-lib")
    add_library(${LIB_TARGET} STATIC IMPORTED)
    # Find the .a outputs from Premake
    set_target_properties(${LIB_TARGET}
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_CURRENT_SOURCE_DIR}/out/android_${RIVE_ARCH}_${CONFIG}/lib${X}.a
    )
    # Link
    target_link_libraries(mprive-android ${LIB_TARGET})
endforeach ()

# =============================================================================
# Link System Libraries
# =============================================================================

# Find additional libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

# Link additional libraries
target_link_libraries(mprive-android
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${gles-lib}
        jnigraphics
)

message(STATUS "✓ CMake configuration complete for mprive-android")