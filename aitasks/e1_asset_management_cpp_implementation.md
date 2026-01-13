# E.1 Asset Management C++ Implementation Task

**Status**: 85% COMPLETE
**Updated**: January 13, 2026

## Completed

### Kotlin Side (100%)
- [x] CommandQueueBridge.kt - 12 asset methods in interface
- [x] CommandQueueBridge.android.kt - 12 external JNI declarations
- [x] CommandQueue.kt - 12 public API methods + 6 callbacks
- [x] Method ID caching for callbacks

### command_server.hpp (100%)
- [x] CommandType entries: DecodeImage, DeleteImage, RegisterImage, UnregisterImage × 3
- [x] MessageType entries: ImageDecoded, ImageError, AudioDecoded, AudioError, FontDecoded, FontError
- [x] Resource maps: m_images, m_audioClips, m_fonts, m_registeredImages, m_registeredAudio, m_registeredFonts
- [x] Public methods: decodeImage, deleteImage, registerImage, unregisterImage × 3
- [x] Handler declarations: handleDecodeImage, etc. (12 handlers)

### command_server.cpp (100%)
- [x] 12 public API methods implemented
- [x] 12 handler implementations (with TODO placeholders for actual decoding)
- [x] Switch cases in executeCommand() for all 12 command types

## Remaining Implementation

### bindings_commandqueue.cpp - JNI Functions (0%)

Add 12 JNI function implementations:
- `cppDecodeImage`, `cppDeleteImage`, `cppRegisterImage`, `cppUnregisterImage`
- `cppDecodeAudio`, `cppDeleteAudio`, `cppRegisterAudio`, `cppUnregisterAudio`
- `cppDecodeFont`, `cppDeleteFont`, `cppRegisterFont`, `cppUnregisterFont`

### bindings_commandqueue.cpp - Message Handling (0%)

Add 6 message handling cases in `cppPollMessages()`:
- `ImageDecoded`, `ImageError`
- `AudioDecoded`, `AudioError`
- `FontDecoded`, `FontError`

## Files Modified This Session
1. `mprive/src/nativeInterop/cpp/src/command_server/command_server.cpp` - Added 12 public methods + 12 handlers

## Files to Modify (Next Session)
1. `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue.cpp`

## Verification
Test compilation: `./gradlew :mprive:compileDebugKotlinAndroid`