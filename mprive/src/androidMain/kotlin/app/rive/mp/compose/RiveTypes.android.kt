package app.rive.mp.compose

import android.graphics.Bitmap

/**
 * Function type for getting a Bitmap from a Rive surface.
 * 
 * This is used with the `onBitmapAvailable` callback in the [Rive] composable
 * to capture rendered frames for snapshot testing or image processing.
 */
typealias GetBitmapFun = () -> Bitmap