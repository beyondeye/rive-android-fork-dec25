package app.rive.mp.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import app.rive.mp.RiveFile
import app.rive.mp.ViewModelInstance

/**
 * Creates a default [ViewModelInstance] from the given [RiveFile].
 *
 * The lifetime of the VMI is managed by this composable. It will delete the VMI when it
 * falls out of scope.
 *
 * Example usage:
 * ```kotlin
 * val riveWorker = rememberRiveWorker()
 * val fileResult = rememberRiveFile(source, riveWorker)
 *
 * when (fileResult) {
 *     is Result.Success -> {
 *         val vmi = rememberViewModelInstance(fileResult.value, "MyViewModel")
 *         // Use VMI with Rive composable for data binding
 *     }
 *     // Handle Loading/Error...
 * }
 * ```
 *
 * @param file The [RiveFile] containing the ViewModel.
 * @param viewModelName The name of the ViewModel to create an instance of.
 * @return The created [ViewModelInstance].
 * @throws IllegalArgumentException If the VMI cannot be created.
 *
 * @see RiveFile For loading a Rive file.
 * @see ViewModelInstance For operations available on the VMI.
 */
@Composable
fun rememberViewModelInstance(
    file: RiveFile,
    viewModelName: String,
): ViewModelInstance {
    val vmi = remember(file, viewModelName) {
        ViewModelInstance.createDefault(file, viewModelName)
    }

    DisposableEffect(vmi) {
        onDispose { vmi.close() }
    }

    return vmi
}

/**
 * Creates a named [ViewModelInstance] from the given [RiveFile].
 *
 * This variant creates a specific named instance of a ViewModel, using the values
 * defined for that instance in the Rive file.
 *
 * The lifetime of the VMI is managed by this composable. It will delete the VMI when it
 * falls out of scope.
 *
 * Example usage:
 * ```kotlin
 * val riveWorker = rememberRiveWorker()
 * val fileResult = rememberRiveFile(source, riveWorker)
 *
 * when (fileResult) {
 *     is Result.Success -> {
 *         val vmi = rememberNamedViewModelInstance(
 *             file = fileResult.value,
 *             viewModelName = "MyViewModel",
 *             instanceName = "instance1"
 *         )
 *         // Use VMI with Rive composable for data binding
 *     }
 *     // Handle Loading/Error...
 * }
 * ```
 *
 * @param file The [RiveFile] containing the ViewModel.
 * @param viewModelName The name of the ViewModel to create an instance of.
 * @param instanceName The name of the specific instance to create.
 * @return The created [ViewModelInstance].
 * @throws IllegalArgumentException If the VMI cannot be created.
 *
 * @see RiveFile For loading a Rive file.
 * @see ViewModelInstance For operations available on the VMI.
 * @see rememberViewModelInstance For creating a default instance.
 */
@Composable
fun rememberNamedViewModelInstance(
    file: RiveFile,
    viewModelName: String,
    instanceName: String,
): ViewModelInstance {
    val vmi = remember(file, viewModelName, instanceName) {
        ViewModelInstance.createNamed(file, viewModelName, instanceName)
    }

    DisposableEffect(vmi) {
        onDispose { vmi.close() }
    }

    return vmi
}

/**
 * Creates a blank [ViewModelInstance] from the given [RiveFile].
 *
 * A blank instance has all properties set to their default values. This is useful
 * when you want to programmatically set all values rather than using values from
 * the Rive file.
 *
 * The lifetime of the VMI is managed by this composable. It will delete the VMI when it
 * falls out of scope.
 *
 * @param file The [RiveFile] containing the ViewModel.
 * @param viewModelName The name of the ViewModel to create an instance of.
 * @return The created [ViewModelInstance].
 * @throws IllegalArgumentException If the VMI cannot be created.
 *
 * @see RiveFile For loading a Rive file.
 * @see ViewModelInstance For operations available on the VMI.
 * @see rememberViewModelInstance For creating a default instance.
 */
@Composable
fun rememberBlankViewModelInstance(
    file: RiveFile,
    viewModelName: String,
): ViewModelInstance {
    val vmi = remember(file, viewModelName) {
        ViewModelInstance.createBlank(file, viewModelName)
    }

    DisposableEffect(vmi) {
        onDispose { vmi.close() }
    }

    return vmi
}