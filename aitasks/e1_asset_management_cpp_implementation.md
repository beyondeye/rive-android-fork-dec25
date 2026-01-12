# E.1 Asset Management C++ Implementation Task

**Status**: IN PROGRESS
**Updated**: January 12, 2026

## Completed

### command_server.hpp
- [x] Added CommandType entries: DecodeImage, DeleteImage, RegisterImage, UnregisterImage, DecodeAudio, DeleteAudio, RegisterAudio, UnregisterAudio, DecodeFont, DeleteFont, RegisterFont, UnregisterFont
- [x] Added MessageType entries: ImageDecoded, ImageError, AudioDecoded, AudioError, FontDecoded, FontError
- [x] Added resource maps: m_images, m_audioClips, m_fonts, m_registeredImages, m_registeredAudio, m_registeredFonts
- [x] Added public methods: decodeImage, deleteImage, registerImage, unregisterImage (x3 for audio/font)
- [x] Added handler declarations: handleDecodeImage, etc. (12 handlers)

## Remaining Implementation

### command_server.cpp
Add asset handler implementations:

```cpp
// =============================================================================
// Phase E.1: Asset Operations - Public API
// =============================================================================

void CommandServer::decodeImage(int64_t requestID, const std::vector<uint8_t>& bytes)
{
    Command cmd(CommandType::DecodeImage, requestID);
    cmd.bytes = bytes;
    enqueueCommand(std::move(cmd));
}

void CommandServer::deleteImage(int64_t imageHandle)
{
    Command cmd(CommandType::DeleteImage, 0);
    cmd.handle = imageHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::registerImage(const std::string& name, int64_t imageHandle)
{
    Command cmd(CommandType::RegisterImage, 0);
    cmd.name = name;
    cmd.handle = imageHandle;
    enqueueCommand(std::move(cmd));
}

void CommandServer::unregisterImage(const std::string& name)
{
    Command cmd(CommandType::UnregisterImage, 0);
    cmd.name = name;
    enqueueCommand(std::move(cmd));
}

// Similar for audio and font...

// =============================================================================
// Phase E.1: Asset Operations - Handlers
// =============================================================================

void CommandServer::handleDecodeImage(const Command& cmd)
{
    // TODO: Implement image decoding using Rive's image decoder
    // For now, return error since we don't have decoder implementation
    Message msg(MessageType::ImageError, cmd.requestID);
    msg.error = "Image decoding not yet implemented";
    enqueueMessage(std::move(msg));
}

// Similar handlers for all 12 operations...
```

### command_server.cpp - Add switch cases in executeCommand()
```cpp
case CommandType::DecodeImage:
    handleDecodeImage(cmd);
    break;
case CommandType::DeleteImage:
    handleDeleteImage(cmd);
    break;
// ... etc for all 12 cases
```

### bindings_commandqueue.cpp - Add 12 JNI functions

```cpp
JNIEXPORT void JNICALL Java_app_rive_mp_core_CommandQueueBridge_cppDecodeImage(
    JNIEnv* env, jobject thiz, jlong serverPtr, jlong requestID, jbyteArray bytes)
{
    auto* server = reinterpret_cast<rive_android::CommandServer*>(serverPtr);
    // Convert jbyteArray to vector
    std::vector<uint8_t> data = jbyteArrayToVector(env, bytes);
    server->decodeImage(requestID, data);
}

// ... etc for all 12 JNI functions
```

### bindings_commandqueue.cpp - Add message handling in cppPollMessages()
```cpp
case MessageType::ImageDecoded:
    env->CallVoidMethod(obj, g_onImageDecoded, msg.requestID, msg.handle);
    break;
case MessageType::ImageError:
    env->CallVoidMethod(obj, g_onImageError, msg.requestID, jError);
    break;
// ... etc for audio and font
```

## Files to Modify
1. `mprive/src/nativeInterop/cpp/src/command_server/command_server.cpp`
2. `mprive/src/nativeInterop/cpp/src/bindings/bindings_commandqueue.cpp`

## Next Session
1. Add handler implementations to command_server.cpp
2. Add switch cases in executeCommand()
3. Add JNI function implementations
4. Add message handling in cppPollMessages()
5. Test compilation with: `./gradlew :mprive:compileDebugKotlinAndroid`