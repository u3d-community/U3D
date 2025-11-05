# Building U3D for macOS

This guide explains how to build U3D for macOS on both Intel and Apple Silicon Macs.

## Prerequisites

- **macOS** 10.15 (Catalina) or later
- **Xcode** (latest version recommended): Install from App Store or [Apple Developer](https://developer.apple.com/xcode/)
- **Xcode Command Line Tools**: `xcode-select --install`
- **CMake** 3.10.2 or later: `brew install cmake`
- **Ruby** (pre-installed on macOS, used for rake build system)

## Quick Start

### 1. Clone and Navigate

```bash
git clone https://github.com/u3d-community/U3D.git
cd U3D
```

### 2. Configure Environment (Optional)

```bash
cp .build-env.template .build-env
# Edit .build-env if needed, then:
source .build-env
```

### 3. Build

```bash
export PLATFORM=macOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode

rake cmake
rake build
```

### 4. Run Samples

```bash
./build/macos/bin/01_HelloWorld
```

## Build Methods

### Method 1: Using Rake (Recommended)

The rake build system provides a simple interface:

```bash
# Configure
export PLATFORM=macOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode
rake cmake

# Build
rake build

# Build specific target
rake build[Urho3D]

# Clean
rake clean

# Install
rake install
```

### Method 2: Using Xcode GUI

```bash
# Generate Xcode project
export PLATFORM=macOS GENERATOR=xcode rake cmake

# Open in Xcode
open build/macos/Urho3D.xcodeproj
```

Then build using Xcode's build button (⌘B).

### Method 3: Using xcodebuild

```bash
cd build/macos
xcodebuild -project Urho3D.xcodeproj -configuration Release
```

### Method 4: Using Make or Ninja

```bash
# Using Make
export GENERATOR=make
rake cmake
cd build/macos
make -j$(sysctl -n hw.ncpu)

# Using Ninja (faster)
export GENERATOR=ninja
rake cmake
cd build/macos
ninja
```

## Configuration Options

### Build Type

```bash
# Debug: Full debugging symbols, no optimizations
export CMAKE_BUILD_TYPE=Debug

# Release: Optimized build (default)
export CMAKE_BUILD_TYPE=Release

# RelWithDebInfo: Release with debug symbols
export CMAKE_BUILD_TYPE=RelWithDebInfo
```

### Library Type

```bash
# Static library (default)
export LIB_TYPE=static

# Shared library (dynamic)
export LIB_TYPE=shared
```

### Feature Flags

Control which features to build in `.build-env`:

```bash
# Build samples (ON by default)
export URHO3D_SAMPLES=ON

# Build player application
export URHO3D_PLAYER=ON

# Scripting support
export URHO3D_ANGELSCRIPT=ON
export URHO3D_LUA=ON
export URHO3D_LUAJIT=ON

# Physics
export URHO3D_PHYSICS=ON
export URHO3D_PHYSICS2D=ON

# Networking
export URHO3D_NETWORK=ON

# 2D support
export URHO3D_URHO2D=ON

# Navigation/AI
export URHO3D_NAVIGATION=ON

# Database support
export URHO3D_DATABASE_SQLITE=ON
export URHO3D_DATABASE_ODBC=OFF

# Profiling
export URHO3D_PROFILING=ON
export URHO3D_TRACY_PROFILING=OFF
```

### Deployment Target

Set the minimum macOS version:

```bash
export CMAKE_OSX_DEPLOYMENT_TARGET=10.15
```

Supported versions: macOS 10.9 and later (10.15+ recommended)

## Code Signing (Optional)

macOS builds don't require code signing for development, but you can enable it:

### Option 1: Environment Variables

```bash
# In .build-env
export MACOS_DEVELOPMENT_TEAM="YOUR_TEAM_ID"
export MACOS_CODE_SIGN_IDENTITY="Apple Development"
```

### Option 2: Manual Signing in Xcode

1. Open `build/macos/Urho3D.xcodeproj`
2. Select target → Signing & Capabilities
3. Configure your team and signing identity

## Architecture Support

### Universal Binaries (Intel + Apple Silicon)

By default, U3D builds for x86_64. To build universal binaries:

```bash
# Edit CMakeLists.txt or UrhoCommon.cmake to set:
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
```

### Apple Silicon Native

On M1/M2/M3 Macs, the default x86_64 build runs through Rosetta 2. For native ARM builds:

```bash
export ARM=1
rake cmake
```

## Advanced Usage

### Custom Install Location

```bash
export CMAKE_INSTALL_PREFIX=/usr/local
rake cmake
rake install
```

### Using U3D in Your Project

After installation, use U3D in your CMake project:

```cmake
find_package(Urho3D REQUIRED)
target_link_libraries(YourApp PRIVATE Urho3D)
```

### Parallel Builds

Speed up builds with parallel jobs:

```bash
# Automatic (uses all CPU cores)
rake build

# Manual
cd build/macos
make -j$(sysctl -n hw.ncpu)
```

### Verbose Build Output

```bash
rake build BUILD_PARAMS="-verbose"
```

## Troubleshooting

### "unknown type name 'Byte'" Error

**Problem**: FreeType fails to compile with `Byte` type errors.

**Solution**: This is fixed in the latest version. The fix ensures `Source/ThirdParty/FreeType/src/gzip/ftzconf.h` line 218 reads:

```c
#if !defined(__MACTYPES__)
typedef unsigned char  Byte;
#endif
```

If you still see this error, update to the latest code:

```bash
git pull origin master
rm -rf build/macos
rake cmake
```

### CMake Not Found

**Problem**: `cmake: command not found`

**Solution**: Install CMake via Homebrew:

```bash
brew install cmake
```

### Xcode License Not Accepted

**Problem**: Build fails with Xcode license agreement message.

**Solution**: Accept the license:

```bash
sudo xcodebuild -license accept
```

### Missing Libraries

**Problem**: Linker errors about missing system frameworks.

**Solution**: Ensure Xcode Command Line Tools are installed:

```bash
xcode-select --install
```

### Build Cache Issues

If you encounter strange build errors, clean and rebuild:

```bash
rm -rf build/macos
rake cmake
rake build
```

## Performance Tips

### Use Ninja for Faster Builds

Ninja is significantly faster than make:

```bash
brew install ninja
export GENERATOR=ninja
rake cmake
cd build/macos && ninja
```

### Enable Precompiled Headers

In `.build-env`:

```bash
export URHO3D_PCH=ON
```

### Use ccache for Incremental Builds

```bash
brew install ccache
export USE_CCACHE=1
rake build
```

## Platform-Specific Features

### Retina Display Support

U3D automatically supports Retina displays. The engine uses backing scale factors for proper rendering.

### Metal Graphics API

macOS builds use OpenGL by default. Metal support may be added in future versions.

### Bundle Creation

To create .app bundles:

```bash
export URHO3D_MACOSX_BUNDLE=ON
rake cmake
```

## Testing

Run the test suite:

```bash
rake test
```

Run specific tests:

```bash
cd build/macos
ctest -R YourTestName -V
```

## Creating Packages

Generate distributable packages:

```bash
rake package
```

This creates `.tar.gz` packages in `build/ci/`.

## Scaffolding New Projects

U3D includes a project scaffolding tool:

```bash
# Install U3D
rake install

# Create new project
rake new

# Your project will be in ~/projects/UrhoApp
cd ~/projects/UrhoApp
rake
```

## Continuous Integration

### GitHub Actions

Example workflow:

```yaml
name: macOS Build
on: [push, pull_request]
jobs:
  macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install CMake
        run: brew install cmake
      - name: Build
        run: |
          export PLATFORM=macOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
          rake cmake
          rake build
```

## See Also

- [Building for iOS](BuildingForIOS.md)
- [Building for Linux](BuildingForLinux.md)
- [Building for Windows](BuildingForWindows.md)
- [Main Build Documentation](../README.md)
- [Contributing Guide](../CONTRIBUTING.md)

## Getting Help

If you encounter issues:

1. Check [GitHub Issues](https://github.com/u3d-community/U3D/issues)
2. Join our [Discord](https://discord.gg/httHCqcXGx)
3. Review [GitHub Discussions](https://github.com/u3d-community/U3D/discussions)

## Common Build Recipes

### Minimal Engine Build

```bash
export URHO3D_SAMPLES=OFF
export URHO3D_PLAYER=OFF
export URHO3D_TOOLS=OFF
export URHO3D_EXTRAS=OFF
rake cmake && rake build
```

### Full-Featured Build

```bash
export URHO3D_ANGELSCRIPT=ON
export URHO3D_LUA=ON
export URHO3D_NETWORK=ON
export URHO3D_PHYSICS=ON
export URHO3D_NAVIGATION=ON
export URHO3D_URHO2D=ON
rake cmake && rake build
```

### Debug Build with Profiling

```bash
export CMAKE_BUILD_TYPE=Debug
export URHO3D_PROFILING=ON
export URHO3D_TRACY_PROFILING=ON
rake cmake && rake build
```
