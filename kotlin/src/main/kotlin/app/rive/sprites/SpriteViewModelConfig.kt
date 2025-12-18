package app.rive.sprites

import app.rive.ViewModelInstance

/**
 * Configuration for creating a [ViewModelInstance] for a [RiveSprite].
 *
 * When creating a sprite, you can specify how its ViewModelInstance should be created
 * (or whether to create one at all). This sealed class provides different options
 * for ViewModel initialization.
 *
 * ## Options
 *
 * - [None] - Don't create a ViewModelInstance (default for sprites without data binding)
 * - [AutoBind] - Use the default ViewModel for the artboard (recommended for most cases)
 * - [Named] - Use a specific ViewModel by name with its default instance
 * - [NamedInstance] - Use a specific ViewModel and instance by name
 * - [External] - Use a pre-created ViewModelInstance
 *
 * ## Example
 *
 * ```kotlin
 * // No ViewModel needed
 * val simpleSprite = scene.createSprite(
 *     file = spriteFile,
 *     viewModelConfig = SpriteViewModelConfig.None
 * )
 *
 * // Auto-bind to default ViewModel (most common)
 * val enemy = scene.createSprite(
 *     file = enemyFile,
 *     viewModelConfig = SpriteViewModelConfig.AutoBind
 * )
 *
 * // Specific ViewModel
 * val boss = scene.createSprite(
 *     file = bossFile,
 *     viewModelConfig = SpriteViewModelConfig.Named("BossViewModel")
 * )
 *
 * // Specific instance
 * val healthyEnemy = scene.createSprite(
 *     file = enemyFile,
 *     viewModelConfig = SpriteViewModelConfig.NamedInstance(
 *         viewModelName = "EnemyViewModel",
 *         instanceName = "FullHealth"
 *     )
 * )
 *
 * // Pre-created instance
 * val customVMI = ViewModelInstance.fromFile(file, source)
 * val customSprite = scene.createSprite(
 *     file = spriteFile,
 *     viewModelConfig = SpriteViewModelConfig.External(customVMI)
 * )
 * ```
 */
sealed class SpriteViewModelConfig {
    /**
     * Don't create a ViewModelInstance for the sprite.
     *
     * Use this for sprites that don't need data binding or when you want
     * to manage the ViewModelInstance separately.
     */
    data object None : SpriteViewModelConfig()

    /**
     * Automatically bind to the default ViewModel for the artboard.
     *
     * This uses the ViewModel that was configured as default in the Rive editor.
     * If no default ViewModel exists, the sprite will have no ViewModelInstance.
     *
     * **Recommended for most use cases** where the artboard has a single
     * associated ViewModel.
     */
    data object AutoBind : SpriteViewModelConfig()

    /**
     * Use a specific ViewModel by name with its default instance.
     *
     * Use this when you know the exact ViewModel name but want to use
     * the default property values configured in the Rive editor.
     *
     * @property viewModelName The name of the ViewModel as defined in the Rive file
     */
    data class Named(val viewModelName: String) : SpriteViewModelConfig()

    /**
     * Use a specific ViewModel and instance by name.
     *
     * Use this when you want to use a pre-configured set of property values
     * from a named instance in the Rive file.
     *
     * @property viewModelName The name of the ViewModel as defined in the Rive file
     * @property instanceName The name of the instance within that ViewModel
     */
    data class NamedInstance(
        val viewModelName: String,
        val instanceName: String
    ) : SpriteViewModelConfig()

    /**
     * Use a pre-created [ViewModelInstance].
     *
     * Use this when you've already created a ViewModelInstance and want to
     * share it with the sprite, or when you need custom initialization.
     *
     * **Note:** The sprite will NOT close the external ViewModelInstance.
     * You are responsible for managing its lifecycle.
     *
     * @property instance The pre-created ViewModelInstance
     */
    data class External(val instance: ViewModelInstance) : SpriteViewModelConfig()

    companion object {
        /**
         * Default configuration - tries to auto-bind if a ViewModel is available.
         */
        val DEFAULT = AutoBind
    }
}
