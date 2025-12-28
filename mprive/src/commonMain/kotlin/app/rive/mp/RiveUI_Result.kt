package app.rive.mp

import androidx.compose.runtime.Composable

/**
 * Represents the result of an operation - typically loading - that can be in a loading, error,
 * or success state. This includes Rive file loading. The Success result must be unwrapped to the
 * value, e.g. through Kotlin's when/is statements.
 */
sealed interface Result<out T> {
    object Loading : Result<Nothing>
    data class Error(val throwable: Throwable) : Result<Nothing>
    data class Success<T>(val value: T) : Result<T>

    @Composable
    fun <T, R> Result<T>.andThen(
        onSuccess: @Composable (T) -> Result<R>
    ): Result<R> = when (this) {
        is Loading -> Loading
        is Error -> Error(this.throwable)
        is Success -> onSuccess(this.value)
    }
}
