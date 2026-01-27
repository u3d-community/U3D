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

import org.gradle.internal.io.NullOutputStream
import org.gradle.internal.os.OperatingSystem

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    `maven-publish`
}

val kotlinVersion: String by rootProject.extra
val ndkSideBySideVersion: String by rootProject.extra
val cmakeVersion: String by rootProject.extra
val buildStagingDir: String by rootProject.extra

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "io.urho3d"
    ndkVersion = ndkSideBySideVersion
    compileSdk = 34
    defaultConfig {
        minSdk = 21
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments.apply {
                    System.getenv("ANDROID_CCACHE")?.let { add("-D ANDROID_CCACHE=$it") }
                    // Pass along matching env-vars as CMake build options
                    addAll(project.file("../../script/.build-options")
                        .readLines()
                        .mapNotNull { variable -> System.getenv(variable)?.let { "-D $variable=$it" } }
                    )
                }
                targets.add("Urho3D")
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
            path = project.file("../../CMakeLists.txt")
            buildStagingDirectory(file("$buildStagingDir/${libType()}"))
        }
    }
    sourceSets {
        named("main") {
            java.srcDir("../../Source/ThirdParty/SDL/android-project/app/src/main/java")
        }
    }
    lint {
        abortOnError = false
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar", "*.aar"))))
    implementation("org.jetbrains.kotlin:kotlin-stdlib:$kotlinVersion")
    implementation("com.getkeepsafe.relinker:relinker:1.4.5")
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test:runner:1.6.2")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
}

androidComponents {
    onVariants { variant ->
        val config = variant.name
        val stagingDir = file("$buildStagingDir/${libType()}")
        afterEvaluate {
            tasks.named<Zip>("bundle${config.replaceFirstChar { it.uppercase() }}Aar").configure {
                // Customize bundle task to also zip the Urho3D headers and libraries
                // AGP 8.x uses <stagingDir>/<BuildType>/<hash>/<abi>/ structure
                val buildTypeDir = stagingDir.resolve(config.replaceFirstChar { it.uppercase() })
                buildTypeDir.listFiles()?.firstOrNull()?.listFiles()?.forEach { abiDir ->
                    if (abiDir.isDirectory) {
                        listOf("include", "lib").forEach { dir ->
                            from(abiDir.resolve(dir)) {
                                into("urho3d/$config/${abiDir.name}/$dir")
                            }
                        }
                    }
                }
            }
        }
    }
}

val stagingDir = file("$buildStagingDir/${libType()}")

tasks {
    register<Delete>("cleanAll") {
        dependsOn("clean")
        delete = setOf(stagingDir)
    }
    register<Jar>("sourcesJar") {
        archiveClassifier.set("sources")
        from(android.sourceSets.getByName("main").java.srcDirs)
    }
    register<Zip>("documentationZip") {
        archiveClassifier.set("documentation")
        dependsOn("makeDoc")
    }
    register<Exec>("makeDoc") {
        // Ignore the exit status on Windows host system because Doxygen may not return exit status correctly on Windows
        isIgnoreExitValue = OperatingSystem.current().isWindows
        standardOutput = NullOutputStream.INSTANCE
        args("--build", ".", "--target", "doc")
        dependsOn("makeDocConfigurer")
    }
    register<Task>("makeDocConfigurer") {
        dependsOn("generateJsonModelRelease")
        doLast {
            // AGP 8.x uses <stagingDir>/<BuildType>/<hash>/<abi>/ structure
            val releaseDir = File(stagingDir, "Release")
            val hashDir = releaseDir.listFiles()?.firstOrNull() ?: error("No Release build directory found")
            val abiDir = hashDir.listFiles()?.firstOrNull { it.isDirectory } ?: error("No ABI directory found")
            val buildTree = abiDir
            named<Exec>("makeDoc") {
                // This is a hack - expect the first line to contain the path to the CMake executable
                executable = File(buildTree, "build_command.txt").readLines().first().split(":").last().trim()
                workingDir = buildTree
            }
            named<Zip>("documentationZip") {
                from(File(buildTree, "Docs/html")) {
                    into("docs")
                }
            }
        }
    }
}

publishing {
    publications {
        android.buildTypes.forEach {
            val config = it.name
            register<MavenPublication>("Urho${config.replaceFirstChar { c -> c.uppercase() }}") {
                configure(config)
            }
        }
    }
    repositories {
        maven {
            name = "GitHubPackages"
            url = uri("https://maven.pkg.github.com/u3d-community/U3D")
            credentials {
                username = System.getenv("GITHUB_ACTOR")
                password = System.getenv("GITHUB_TOKEN")
            }
        }
    }
}

fun libType(): String {
    return System.getenv("URHO3D_LIB_TYPE")?.lowercase() ?: "static"
}

fun MavenPublication.configure(config: String) {
    groupId = project.group.toString()
    artifactId = "${project.name}-${libType()}${if (config == "debug") "-debug" else "" }"
    afterEvaluate {
        from(components[config])
    }
    artifact(tasks["sourcesJar"])
    pom {
        inceptionYear.set("2008")
        licenses {
            license {
                name.set("MIT License")
                url.set("https://github.com/u3d-community/U3D/blob/master/LICENSE")
            }
        }
        developers {
            developer {
                name.set("Urho3D contributors")
                url.set("https://github.com/u3d-community/U3D/graphs/contributors")
            }
        }
        scm {
            url.set("https://github.com/u3d-community/U3D.git")
            connection.set("scm:git:ssh://git@github.com:u3d-community/U3D.git")
            developerConnection.set("scm:git:ssh://git@github.com:u3d-community/U3D.git")
        }
        withXml {
            asNode().apply {
                appendNode("name", "Urho3D")
                appendNode("description", project.description)
                appendNode("url", "https://urho3d.io/")
            }
        }
    }
}
