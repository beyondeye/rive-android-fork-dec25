package app.rive.sprites.matrix

/**
 * Lightweight matrix representation using a 9-element FloatArray.
 * 
 * This value class provides an efficient way to represent affine transformations
 * with minimal allocations. The matrix is stored in a 3x3 format (9 elements) and
 * provides lazy conversion to platform-specific Matrix objects only when needed
 * for operations like inversion or point transformation.
 * 
 * The 9 elements are in row-major order:
 * ```
 * [scaleX, skewX, transX]
 * [skewY, scaleY, transY]
 * [persp0, persp1, persp2]
 * ```
 * 
 * For 2D affine transformations, the perspective row is always [0, 0, 1].
 */
@JvmInline
value class TransMatrixData(private val values: FloatArray) {
    init {
        require(values.size == 9) { "Matrix must be 9 elements, got ${values.size}" }
    }

    fun asMatrix(): TransMatrix  {
        return TransMatrix().apply {
            setValues(values)
        }
    }
    
    /**
     * Get the raw 9-element array.
     * 
     * This provides direct access to the underlying FloatArray without any
     * conversion overhead, useful for passing to native code or custom operations.
     */
    fun getValues(): FloatArray = values
}
