plugins {
    alias(fork.plugins.kotlin.multiplatform)
    alias(fork.plugins.android.library)
    alias(fork.plugins.compose.multiplatform)
    alias(fork.plugins.compose.compiler)
}

kotlin {
    // Android target
    androidTarget {
        compilerOptions {
            jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_11)
        }
    }
    
    // Desktop JVM target
    jvm("desktop") {
        compilerOptions {
            jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_11)
        }
    }
    
    // iOS targets
    listOf(
        iosX64(),
        iosArm64(),
        iosSimulatorArm64()
    ).forEach { iosTarget ->
        iosTarget.binaries.framework {
            baseName = "mprive"
            isStatic = true
        }
    }
    
    // Wasm/JS target
    @OptIn(org.jetbrains.kotlin.gradle.ExperimentalWasmDsl::class)
    wasmJs {
        browser()
    }
    
    sourceSets {
        // Common source set
        val commonMain by getting {
            dependencies {
                // Compose Multiplatform - for @Immutable and @Composable support
                implementation(fork.jetbrains.compose.runtime)
                
                // Compose Multiplatform UI - for Modifier, Layout, pointer input, etc.
                implementation(compose.ui)
                implementation(compose.foundation)
                implementation(compose.material)
                
                // Compose Multiplatform Lifecycle - for LocalLifecycleOwner, repeatOnLifecycle, etc.
                implementation(fork.jetbrains.lifecycle.runtime.compose)
                
                // Coroutines - for suspend functions
                implementation(fork.kotlinx.coroutines.core)
                
                // AtomicFU - for atomic operations in multiplatform
                implementation(fork.atomicfu)
            }
        }
        
        val commonTest by getting {
            dependencies {
                implementation(kotlin("test"))
                implementation(fork.kotlinx.coroutines.test)
                implementation(compose.runtime)
            }
        }
        
        // Android source set
        val androidMain by getting {
            dependencies {
                // Android-specific dependencies
            }
        }
        
        // Android test source set
        // ⚠️ WORKAROUND: dependsOn(commonTest) triggers Gradle warnings but is necessary
        // ISSUE: Android instrumented tests (androidInstrumentedTest) and common tests (commonTest)
        //        are in different "Source Set Trees" in KMP's hierarchy. KMP considers:
        //        - Main tree: commonMain → androidMain, desktopMain, iosMain
        //        - Test tree: commonTest → desktopTest, iosTest
        //        - Instrumented tree: androidInstrumentedTest (separate, requires Android runtime)
        //
        // The dependsOn(commonTest) below allows sharing expect/actual test utilities between
        // commonTest and Android instrumented tests, but Gradle warns:
        //   "Invalid Source Set Dependency Across Trees"
        //   "Default Kotlin Hierarchy Template Not Applied Correctly"
        //
        // ALTERNATIVES CONSIDERED (all have drawbacks):
        //   1. Accept warnings - code compiles and works (CURRENT APPROACH)
        //   2. Duplicate code - maintain separate test utilities per platform
        //   3. Move to main - pollutes production code with test utilities
        //   4. Separate module - adds complexity for small utility classes
        //
        // STATUS: No ideal solution exists for sharing test utilities with Android instrumented
        //         tests in KMP. This workaround is acceptable until KMP improves support.
        val androidInstrumentedTest by getting {
            dependsOn(commonTest)  // Link to commonTest for expect/actual
            dependencies {
                implementation(kotlin("test"))
                implementation(fork.androidx.core)
                implementation(fork.androidx.junit.v115)
                implementation(fork.androidx.runner.v152)
                implementation(fork.androidx.ui.test.junit4)
                implementation(fork.activity.compose)
            }
        }
        
        // Desktop JVM source set
        val desktopMain by getting {
            dependencies {
                // Desktop-specific dependencies
            }
        }
        
        // Desktop test source set
        val desktopTest by getting {
            dependencies {
                implementation(kotlin("test"))
                implementation(compose.desktop.currentOs)
                implementation(compose.desktop.uiTestJUnit4)
            }
        }
        
        // Wasm/JS source set
        val wasmJsMain by getting {
            dependencies {
                // Wasm/JS dependencies
            }
        }
    }
    
    // Configure iOS source set (auto-created by default hierarchy template)
    // Using matching to safely configure the source set if it exists
    sourceSets.matching { it.name == "iosMain" }.configureEach {
        dependencies {
            // Add iOS-specific dependencies here
        }
    }
}

android {
    namespace = "app.rive.mprive"
    compileSdk = 36

    defaultConfig {
        minSdk = 21
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    // Configure test source sets to include commonTest resources as assets
    // This allows Android instrumented tests to access shared test resources
    sourceSets {
        getByName("androidTest") {
            assets.srcDirs("src/commonTest/resources")
        }
    }
    
    // Native build configuration for Android (similar to kotlin module)
    ndkVersion = "27.2.12479018"
    
    externalNativeBuild {
        cmake {
            path = file("src/androidMain/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    
    defaultConfig {
        externalNativeBuild {
            cmake {
                abiFilters("x86", "x86_64", "armeabi-v7a", "arm64-v8a")
                arguments(
                    "-DCMAKE_VERBOSE_MAKEFILE=1",
                    "-DANDROID_ALLOW_UNDEFINED_SYMBOLS=ON",
                    "-DANDROID_CPP_FEATURES=no-exceptions no-rtti",
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
                )
            }
        }
    }
}