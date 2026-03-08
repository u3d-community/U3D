//
// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2024 the U3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val kotlinVersion: String by rootProject.extra
val ndkSideBySideVersion: String by rootProject.extra
val cmakeVersion: String by rootProject.extra
val buildStagingDir: String by rootProject.extra

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "io.urho3d.launcher"
    ndkVersion = ndkSideBySideVersion
    compileSdk = 34

    // android : the launcher-app has always an shared build
    if (libType() == "shared")
    {
        defaultConfig {
            minSdk = 21
            targetSdk = 34
            applicationId = "io.urho3d.launcher"
            versionCode = 1
            versionName = project.version.toString()
            testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
            externalNativeBuild {
                cmake {
                    arguments.apply {
                        System.getenv("ANDROID_CCACHE")?.let { add("-D ANDROID_CCACHE=$it") }
                        add("-D BUILD_STAGING_DIR=${findProject(":android:urho3d-lib")!!.projectDir}/$buildStagingDir/shared")
                        add("-D URHO3D_PLAYER=1")
                        add("-D URHO3D_SAMPLES=1")
                        // Pass along matching env-vars as CMake build options
                        addAll(project.file("../../script/.build-options")
                            .readLines()
                            .filterNot { listOf("URHO3D_PLAYER", "URHO3D_SAMPLES").contains(it) }
                            .mapNotNull { variable -> System.getenv(variable)?.let { "-D $variable=$it" } }
                        )
                    }
                }
            }
            splits {
                abi {
                    isEnable = project.hasProperty("ANDROID_ABI")
                    reset()
                    include(
                        *(project.findProperty("ANDROID_ABI") as String? ?: "")
                            .split(',')
                            .toTypedArray()
                    )
                }
            }
        }
        buildTypes {
            named("release") {
                isMinifyEnabled = false
                proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            }
        }
        externalNativeBuild {
            cmake {
                version = cmakeVersion
                path = project.file("CMakeLists.txt")
                buildStagingDirectory(file(buildStagingDir))
            }
        }
        lint {
            abortOnError = false
        }
    }
}

dependencies {
    implementation(project(":android:urho3d-lib"))
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar", "*.aar"))))
    implementation("org.jetbrains.kotlin:kotlin-stdlib:$kotlinVersion")
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test:runner:1.6.2")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
}

if (libType() == "shared")
{
    afterEvaluate {
        android.buildTypes.forEach {
            val config = it.name.replaceFirstChar { c -> c.uppercase() }
            tasks {
                "externalNativeBuild$config" {
                    dependsOn(":android:urho3d-lib:externalNativeBuild$config")
                }
            }
        }
    }
    tasks {
        register<Delete>("cleanAll") {
            dependsOn("clean")
            delete = setOf(file(buildStagingDir))
        }
    }
}
else {
    afterEvaluate {
        tasks.configureEach {
            enabled = false
        }
        println("the launcher-app build cannot be done with 'static' library type.")
        println("To build it, type in a terminal : URHO3D_LIB_TYPE=SHARED ./gradlew build (linux)")
        println("                                  \$env:URHO3D_LIB_TYPE=\"SHARED\"; ./gradlew build (win)")
    }
}

fun libType(): String {
    return System.getenv("URHO3D_LIB_TYPE")?.lowercase() ?: "static"
}
