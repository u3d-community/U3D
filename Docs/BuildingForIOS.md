# Building U3D for iOS

This guide explains how to build U3D for iOS devices and the iOS Simulator on macOS.

## Prerequisites

- **macOS** (10.15 or later recommended)
- **Xcode** (latest version recommended)
- **CMake** 3.10.2 or later: `brew install cmake`
- **Ruby** (for rake build system, pre-installed on macOS)
- **Apple Developer Account** (required for device builds, optional for simulator)

## Quick Start

### Option A: iOS Simulator (No Code Signing Required!) ⭐

Perfect for development and testing without an Apple Developer account:

```bash
export PLATFORM=iOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode
export IOS_SIMULATOR_ONLY=1

rake cmake
rake build
```

That's it! The build will work on iOS Simulator without any code signing setup.

### Option B: iOS Device (Requires Apple Developer Account)

For building to run on physical iOS devices:

1. Configure Build Environment

Copy the environment template and configure code signing:

```bash
cp .build-env.template .build-env
```

Edit `.build-env` and set your Apple Development Team ID:

```bash
# Find your Team ID at: https://developer.apple.com/account
export IOS_DEVELOPMENT_TEAM="YOUR_TEAM_ID_HERE"
export IOS_CODE_SIGN_IDENTITY="Apple Development"
```

Then source the environment:

```bash
source .build-env
```

2. Build for iOS

#### iOS Device Build (requires code signing)

```bash
export PLATFORM=iOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode

rake cmake
rake build
```

#### iOS Simulator Build

Same commands as above - the Xcode project includes both device and simulator configurations.

### 3. Open in Xcode (Optional)

```bash
open build/ios/Urho3D.xcodeproj
```

You can then build and run samples directly from Xcode.

## Detailed Configuration

### iOS Simulator-Only Mode (Recommended for Development)

Build for iOS Simulator without any Apple Developer account:

```bash
export IOS_SIMULATOR_ONLY=1
export PLATFORM=iOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
rake cmake
rake build
```

**Benefits**:
- ✅ No Apple Developer account required
- ✅ No code signing setup needed
- ✅ Fast iteration for development
- ✅ Works immediately out of the box

**Limitations**:
- ⚠️ Only builds for iOS Simulator (not physical devices)
- ⚠️ Project configured for simulator SDK only

**Running on Simulator**:
```bash
# Open in Xcode and run on simulator
open build/ios/Urho3D.xcodeproj

# Or use command line with xcrun simctl
```

### Code Signing Options

U3D supports four code signing modes:

#### 0. Simulator-Only Mode (No Signing - Easiest!)

Set `IOS_SIMULATOR_ONLY=1` environment variable. See section above for details.

#### 1. Environment-based Signing (For Device Builds)

Configure via `.build-env`:

```bash
# Required: Your Apple Development Team ID
export IOS_DEVELOPMENT_TEAM="ABCDEFGHIJ"

# Optional: Code signing identity (defaults to "Apple Development")
export IOS_CODE_SIGN_IDENTITY="Apple Development"

# Optional: Provisioning profile specifier
export IOS_PROVISIONING_PROFILE_SPECIFIER=""

# Optional: Bundle identifier prefix
export IOS_BUNDLE_ID_PREFIX="com.yourcompany.u3d"
```

#### 2. Manual Signing in Xcode

If you don't configure environment variables, you can manually set code signing in Xcode:

1. Open `build/ios/Urho3D.xcodeproj`
2. Select each target you want to build
3. Go to "Signing & Capabilities"
4. Select your team and configure signing

#### 3. Library-Only Build (No Signing Required)

To build only the engine library without samples:

```bash
export URHO3D_SAMPLES=OFF
export URHO3D_PLAYER=OFF
rake cmake
rake build
```

### Build Configurations

The Xcode project includes these configurations:

- **Debug**: Full debugging symbols, no optimizations
- **Release**: Optimized build for distribution
- **RelWithDebInfo**: Release with debug information

Select the configuration when building:

```bash
rake build          # Uses default (Release)
CMAKE_BUILD_TYPE=Debug rake cmake && rake build
```

### Deployment Targets

Set the minimum iOS version in `.build-env`:

```bash
export IPHONEOS_DEPLOYMENT_TARGET="12.0"
```

Supported versions: iOS 12.0 and later

## Building from Command Line

### Build Specific Targets

```bash
# Build only the engine library
cd build/ios
xcodebuild -project Urho3D.xcodeproj -target Urho3D -configuration Release

# Build a specific sample
xcodebuild -project Urho3D.xcodeproj -target 01_HelloWorld -configuration Release
```

### Build for Specific SDK

```bash
# iOS Device
xcodebuild -project Urho3D.xcodeproj -sdk iphoneos -configuration Release

# iOS Simulator
xcodebuild -project Urho3D.xcodeproj -sdk iphonesimulator -configuration Release
```

## Troubleshooting

### "Signing requires a development team"

**Problem**: Build fails with signing errors.

**Solutions**:
1. Set `IOS_DEVELOPMENT_TEAM` environment variable (see above)
2. Manually configure signing in Xcode
3. Build library-only without samples

### "No provisioning profiles found"

**Problem**: Xcode can't find provisioning profiles.

**Solutions**:
1. Let Xcode automatically manage signing:
   - Open the project in Xcode
   - Enable "Automatically manage signing"
   - Xcode will create profiles automatically

2. Manually create profiles at [Apple Developer Portal](https://developer.apple.com/account/resources/profiles)

### "Unknown type name 'Byte'" in FreeType

**Problem**: Build fails in FreeType with undefined `Byte` type.

**Solution**: This is fixed in the latest version. If you encounter this error, ensure you have the latest code or check that `Source/ThirdParty/FreeType/src/gzip/ftzconf.h` line 218 reads:
```c
#if !defined(__MACTYPES__)
typedef unsigned char  Byte;
#endif
```

### CMake regeneration required

If you change code signing settings, you need to regenerate the project:

```bash
rm -rf build/ios
rake cmake
```

## Advanced Usage

### Cross-Compilation Options

U3D uses CMake cross-compilation for iOS. Key variables:

- `CMAKE_OSX_SYSROOT`: Set to `iphoneos` for device builds
- `CMAKE_XCODE_EFFECTIVE_PLATFORMS`: Includes both device and simulator
- `IPHONEOS_DEPLOYMENT_TARGET`: Minimum iOS version

### Custom Build Scripts

Create a custom build script:

```bash
#!/bin/bash
source .build-env

export PLATFORM=iOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode

# Clean previous build
rm -rf build/ios

# Generate and build
rake cmake
rake build

echo "iOS build complete!"
```

### Continuous Integration

For CI builds (GitHub Actions, etc.), code signing is automatically disabled. No special configuration needed.

## Platform-Specific Notes

### iOS Simulator on Apple Silicon

On M1/M2/M3 Macs, the iOS Simulator runs ARM64 natively. U3D builds universal binaries that work on both architectures.

### iOS 15+ Requirements

For iOS 15 and later, ensure your Xcode version is 13.0 or newer.

## See Also

- [Building for macOS](BuildingForMacOS.md)
- [Building for tvOS](BuildingForTVOS.md)
- [Main Build Documentation](../README.md)
- [Contributing Guide](../CONTRIBUTING.md)

## Getting Help

If you encounter issues:

1. Check the [GitHub Issues](https://github.com/u3d-community/U3D/issues)
2. Join our [Discord](https://discord.gg/httHCqcXGx)
3. Review [GitHub Discussions](https://github.com/u3d-community/U3D/discussions)
