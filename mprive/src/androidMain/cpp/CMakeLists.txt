cmake_minimum_required(VERSION 3.18.1)

project(mprive-android VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_VERBOSE_MAKEFILE ON)

# Find all source files
file(GLOB SOURCES CONFIGURE_DEPENDS
        src/*.cpp
)

# Create the mprive-android library target
add_library(mprive-android SHARED ${SOURCES})

# Enable C++17
target_compile_features(mprive-android PUBLIC cxx_std_17)
target_compile_options(mprive-android PRIVATE
        -Wall
        -fno-exceptions
        -fno-rtti
        $<$<CONFIG:Release>:-Oz>)
target_compile_definitions(mprive-android PRIVATE
        MPRIVE_ANDROID=
        $<$<CONFIG:Debug>:DEBUG>)

# Configure for rive-runtime directory
if (EXISTS "${PROJECT_SOURCE_DIR}/../../../../../submodules/rive-runtime")
    set(PACKAGES_DIR "${PROJECT_SOURCE_DIR}/../../../../../")
    set(RIVE_RUNTIME_DIR "${PACKAGES_DIR}/submodules/rive-runtime")
else()
    set(PACKAGES_DIR "${PROJECT_SOURCE_DIR}/../../../../../../..")
    set(RIVE_RUNTIME_DIR "${PACKAGES_DIR}/runtime")
endif()

# Add Rive runtime headers
target_include_directories(mprive-android PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${RIVE_RUNTIME_DIR}/include
        ${RIVE_RUNTIME_DIR}/renderer/include)

# Find Android libraries
find_library(log-lib log)
find_library(android-lib android)

# Link Android libraries
target_link_libraries(mprive-android
        ${log-lib}
        ${android-lib}
)
