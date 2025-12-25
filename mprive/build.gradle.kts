plugins {
    alias(fork.plugins.kotlin.multiplatform)
    alias(fork.plugins.android.library)
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
                // Add common dependencies here
            }
        }
        
        val commonTest by getting {
            dependencies {
                implementation(kotlin("test"))
            }
        }
        
        // Android source set
        val androidMain by getting {
            dependencies {
                // Android-specific dependencies
            }
        }
        
        // Desktop JVM source set
        val desktopMain by getting {
            dependencies {
                // Desktop-specific dependencies
            }
        }
        
        // iOS source set
        val iosX64Main by getting
        val iosArm64Main by getting
        val iosSimulatorArm64Main by getting
        val iosMain by creating {
            dependsOn(commonMain)
            iosX64Main.dependsOn(this)
            iosArm64Main.dependsOn(this)
            iosSimulatorArm64Main.dependsOn(this)
        }
        
        // Wasm/JS source set
        val wasmJsMain by getting {
            dependencies {
                // Wasm/JS dependencies
            }
        }
    }
}

android {
    namespace = "app.rive.mprive"
    compileSdk = 36
    
    defaultConfig {
        minSdk = 21
    }
    
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
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
