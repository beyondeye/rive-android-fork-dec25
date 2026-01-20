#include "render_context.hpp"
#include "helpers/android_factory.hpp"
#include "rive_log.hpp"

namespace rive_mp
{

void RenderContextGL::createAndroidFactory()
{
    LOGD("[RenderContext] Creating AndroidFactory for platform-native image decoding");
    
    // Create AndroidFactory that wraps this RenderContext
    // The factory will use Android's BitmapFactory for image decoding
    // and delegate all other operations to the underlying riveContext
    m_androidFactory = std::make_unique<AndroidFactory>(this);
    
    if (m_androidFactory)
    {
        LOGD("[RenderContext] AndroidFactory created successfully");
    }
    else
    {
        LOGE("[RenderContext] Failed to create AndroidFactory");
    }
}

} // namespace rive_mp