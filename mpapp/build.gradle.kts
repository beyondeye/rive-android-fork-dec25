plugins {
    alias(fork.plugins.kotlin.multiplatform)
    alias(fork.plugins.android.application)
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
            baseName = "mpapp"
            isStatic = true
        }
    }
    
    // Wasm/JS target
    @OptIn(org.jetbrains.kotlin.gradle.ExperimentalWasmDsl::class)
    wasmJs {
        browser()
        binaries.executable()
    }
    
    sourceSets {
        // Common source set
        val commonMain by getting {
            dependencies {
                implementation(project(":mprive"))
                implementation(compose.runtime)
                implementation(compose.foundation)
                implementation(compose.material3)
                implementation(compose.ui)
                implementation(compose.components.resources)
            }
        }
        
        // Android source set
        val androidMain by getting {
            dependencies {
                implementation(fork.androidx.activity.compose)
                implementation(fork.androidx.appcompat)
                implementation(fork.androidx.core.ktx)
            }
        }
        
        // Desktop JVM source set
        val desktopMain by getting {
            dependencies {
                implementation(compose.desktop.currentOs)
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
        val wasmJsMain by getting
    }
}

android {
    namespace = "app.rive.mpapp"
    compileSdk = 36
    
    defaultConfig {
        applicationId = "app.rive.mpapp"
        minSdk = 21
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"
    }
    
    buildTypes {
        debug {
            isDebuggable = true
            isMinifyEnabled = false
        }
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    
    buildFeatures {
        compose = true
    }
}

compose.desktop {
    application {
        mainClass = "app.rive.mpapp.MainKt"
    }
}
