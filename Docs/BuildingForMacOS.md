# Building U3D for macOS

Guide for building U3D natively on macOS (Intel and Apple Silicon).

**→ Back to [Apple Platforms Overview](../APPLE_README.md)**

## Prerequisites

- **macOS** 10.15 (Catalina) or later
- **Xcode** (latest version): Install from App Store
- **Xcode Command Line Tools**: `xcode-select --install`
- **CMake** 3.10.2+: `brew install cmake`

## Quick Start

```bash
# From repo root
export PLATFORM=macOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
export URHO3D_ANGELSCRIPT=0 URHO3D_LUA=0 URHO3D_OPENGL=1
export CMAKE_OSX_ARCHITECTURES=arm64
rake cmake

# Build
cd build/macos
xcodebuild -target 01_HelloWorld -configuration Release CODE_SIGN_IDENTITY="-"

# Run
./bin/01_HelloWorld
```

**No code signing required!** macOS allows running locally built apps without signing.

## Build Methods

### Method 1: Command Line (Recommended)

**Build specific target:**
```bash
# From repo root
cd build/macos
xcodebuild -target 01_HelloWorld -configuration Release CODE_SIGN_IDENTITY="-"
./bin/01_HelloWorld
```

**Build all samples:**
```bash
cd build/macos
xcodebuild -scheme ALL_BUILD -configuration Release CODE_SIGN_IDENTITY="-"
```

### Method 2: Xcode GUI

```bash
# From repo root
# Generate project
export PLATFORM=macOS GENERATOR=xcode
rake cmake

# Open in Xcode
open build/macos/Urho3D.xcodeproj
```

In Xcode:
1. Select a target (e.g., `01_HelloWorld`)
2. Select "My Mac" as destination
3. Click Run (⌘R) or Build (⌘B)

If Xcode asks for signing, click "Sign to Run Locally" (it's free).

## Architecture Support

### Apple Silicon (arm64) - Recommended

```bash
export CMAKE_OSX_ARCHITECTURES=arm64
rake cmake
```

**Why arm64 only?**
- ✅ Avoids x86_64 AngelScript build errors
- ✅ Native performance on Apple Silicon
- ✅ Faster builds

### Intel (x86_64)

```bash
export CMAKE_OSX_ARCHITECTURES=x86_64
export URHO3D_ANGELSCRIPT=0  # Disable AngelScript (has x86_64 issues)
rake cmake
```

### Universal Binary (Both)

```bash
# Not recommended due to AngelScript x86_64 issues
# Build separately and use lipo to combine if needed
```

## Build Configuration

### Recommended Settings

```bash
# From repo root
export PLATFORM=macOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode
export CMAKE_BUILD_TYPE=Release

# Disable scripting (has build issues)
export URHO3D_ANGELSCRIPT=0
export URHO3D_LUA=0

# Use desktop OpenGL
export URHO3D_OPENGL=1

# Build for Apple Silicon only
export CMAKE_OSX_ARCHITECTURES=arm64
```

### Build Types

```bash
# Release (optimized, recommended)
export CMAKE_BUILD_TYPE=Release

# Debug (with symbols)
export CMAKE_BUILD_TYPE=Debug

# RelWithDebInfo (optimized + symbols)
export CMAKE_BUILD_TYPE=RelWithDebInfo
```

## Common Commands

```bash
# From repo root

# Clean build
rm -rf build/macos

# Reconfigure
rake cmake

# Build specific target
cd build/macos
xcodebuild -target 01_HelloWorld -configuration Release CODE_SIGN_IDENTITY="-"

# Run
./bin/01_HelloWorld

# Build all
xcodebuild -scheme ALL_BUILD -configuration Release CODE_SIGN_IDENTITY="-"
```

## Troubleshooting


### AngelScript build errors (x86_64)

```
CompileC .../as_callfunc_x64_gcc.o .../as_callfunc_x64_gcc.cpp normal x86_64
```

**Solution**: Build arm64 only or disable AngelScript:
```bash
export CMAKE_OSX_ARCHITECTURES=arm64
# OR
export URHO3D_ANGELSCRIPT=0
rake cmake
```

### "An empty code signing identity is not valid"

Use ad-hoc signing:
```bash
xcodebuild -target 01_HelloWorld CODE_SIGN_IDENTITY="-"
```

### Build is slow

```bash
# Build single architecture (faster)
export CMAKE_OSX_ARCHITECTURES=arm64

# Use Release build (not Debug)
export CMAKE_BUILD_TYPE=Release

# Disable unused features
export URHO3D_ANGELSCRIPT=0 URHO3D_LUA=0
```

## Performance Tips

### For Development
- Use **Release** builds for normal development
- Use **Debug** only when debugging
- Build **arm64 only** (not universal)

### For Distribution
- Build with optimizations: `CMAKE_BUILD_TYPE=Release`
- Sign with Developer ID certificate

## Code Signing Notes

### For Local Development
- ✅ **No signing required** - just build and run
- ✅ Works with ad-hoc signing: `CODE_SIGN_IDENTITY="-"`

### For Distribution
- ⚠️ **Signing required** for App Store or public distribution
- Need Developer ID certificate
- Need to notarize with Apple

## Build Environment Template

```bash
# From repo root
cp .build-env.template .build-env
# Edit .build-env with your settings
source .build-env
rake cmake
cd build/macos
xcodebuild -target 01_HelloWorld -configuration Release CODE_SIGN_IDENTITY="-"
```

Example `.build-env`:
```bash
export PLATFORM=macOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode
export CMAKE_BUILD_TYPE=Release
export URHO3D_ANGELSCRIPT=0
export URHO3D_LUA=0
export URHO3D_OPENGL=1
export CMAKE_OSX_ARCHITECTURES=arm64
```

## What Works

- ✅ All rendering (OpenGL via GLEW)
- ✅ Full GPU acceleration
- ✅ Audio, networking, file I/O
- ✅ All system APIs

## Current Limitations

- ⚠️ **AngelScript/Lua**: Disabled due to build issues
- ⚠️ **x86_64**: AngelScript has compilation errors
- ⚠️ **Universal Binary**: Build architectures separately

## Running Samples

All 54 samples work on macOS:

```bash
# From repo root
cd build/macos/bin

# Run any sample
./01_HelloWorld
./02_HelloGUI
./06_SkeletalAnimation
./19_VehicleDemo
```

## See Also

- **[../APPLE_README.md](../APPLE_README.md)** - Apple platforms overview
- **[BuildingForIOS.md](BuildingForIOS.md)** - iOS build guide

---

**Last Updated**: 2025-11-06
