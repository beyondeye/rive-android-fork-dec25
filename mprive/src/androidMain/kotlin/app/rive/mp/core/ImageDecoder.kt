package app.rive.mp.core

import android.graphics.BitmapFactory

/**
 * Image decoder utility that uses Android's BitmapFactory to decode images.
 * Called from C++ via JNI to decode embedded images in Rive files.
 * 
 * This is mprive's independent implementation, adapted from kotlin module's
 * ImageDecoder to keep mprive self-contained without dependency on the
 * kotlin module.
 */
@Suppress("unused") // Called from JNI
object ImageDecoder {
    /**
     * Decodes a byte array into a bitmap and returns an integer array containing the pixel data.
     *
     * Used by C++ over JNI to decode images during Rive file import.
     *
     * @param encoded The byte array containing the encoded image data.
     * @return An array of integers where the first two elements are the width and height of the
     *    bitmap, followed by the pixel data in ARGB, non-premultiplied format.
     *    Returns empty array on failure.
     */
    @JvmStatic
    fun decodeToBitmap(encoded: ByteArray): IntArray {
        return try {
            val bitmap =
                BitmapFactory.decodeByteArray(encoded, 0, encoded.size, BitmapFactory.Options())
                    ?: return IntArray(0)

            val width = bitmap.width
            val height = bitmap.height
            val offset = 2 // Space for width and height
            val pixels = IntArray(offset + width * height)
            pixels[0] = width
            pixels[1] = height
            bitmap.getPixels(pixels, offset, width, 0, 0, width, height)
            pixels
        } catch (e: Exception) {
            IntArray(0)
        }
    }
}