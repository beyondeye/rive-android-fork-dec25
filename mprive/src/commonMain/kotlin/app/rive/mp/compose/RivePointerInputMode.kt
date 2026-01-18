package app.rive.mp.compose

/**
 * Controls how a Rive composable participates in Compose pointer input dispatch.
 * 
 * This enum determines how pointer events (touch, mouse, etc.) are handled and whether
 * they are consumed or passed through to other composables.
 */
enum class RivePointerInputMode {
    /**
     * Rive handles pointer events and consumes them, preventing parent/ancestor gesture
     * detectors (e.g., scroll) from also acting.
     * 
     * This is the default mode and is appropriate for most use cases where the Rive
     * animation is the primary interactive element.
     */
    Consume,
    
    /**
     * Rive handles pointer events but does not consume them. Parent/ancestor gesture
     * detectors may also react to the same events.
     * 
     * Use this mode when you want the Rive animation to respond to input while still
     * allowing parent scrolling or other gestures to work.
     */
    Observe,
    
    /**
     * Rive handles pointer events and also shares them with any sibling composables
     * positioned underneath it without consuming.
     * 
     * This is useful if your Rive file is an overlay with transparent sections that
     * should allow pointer events through to underlying interactive elements.
     */
    PassThrough
}