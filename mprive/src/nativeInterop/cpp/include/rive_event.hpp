#ifndef RIVE_ANDROID_RIVE_EVENT_HPP
#define RIVE_ANDROID_RIVE_EVENT_HPP

#include <cstdint>
#include <string>
#include <map>
#include <variant>

namespace rive_android {

/**
 * Event type enum matching the Rive runtime type keys.
 *
 * Note: Only user-facing event types are included.
 * Internal types (ListenerFireEvent=168, StateMachineFireEvent=169) are not exposed.
 */
enum class RiveEventType : uint16_t {
    GeneralEvent = 128,
    OpenURLEvent = 131,
    AudioEvent = 407
};

/**
 * Variant for event property values.
 * Matches the property types supported by Rive events.
 */
using EventPropertyValue = std::variant<
    bool,
    float,
    std::string
>;

/**
 * Data structure for transferring event data from C++ to Kotlin.
 *
 * This is a pure data structure (no pointers) that can be safely
 * passed across the JNI boundary.
 */
struct RiveEventData {
    std::string name;
    RiveEventType type = RiveEventType::GeneralEvent;
    float delay = 0.0f;

    // OpenURLEvent specific
    std::string url;
    uint32_t targetValue = 0;  // 0=_blank, 1=_parent, 2=_self, 3=_top

    // AudioEvent specific
    uint32_t assetId = 0;

    // Custom properties
    std::map<std::string, EventPropertyValue> properties;
};

} // namespace rive_android

#endif // RIVE_ANDROID_RIVE_EVENT_HPP
