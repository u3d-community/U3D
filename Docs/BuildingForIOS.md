# Building U3D for iOS

Guide for building U3D for iOS Simulator and iOS Devices.

**→ Back to [Apple Platforms Overview](../APPLE_README.md)**

## Prerequisites

- **macOS** 10.15+
- **Xcode** (latest version)
- **CMake** 3.10.2+: `brew install cmake`
- **Apple Developer Account** (only for device builds)

## iOS Simulator (No Code Signing!)

Perfect for development without an Apple Developer account.

### Build

```bash
# From repo root
export PLATFORM=iOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
export IOS_SIMULATOR_ONLY=1
rake cmake
rake build
```

### Run on Simulator

```bash
# Boot simulator (if not already running)
open -a Simulator

# Install app
cd build/ios
xcrun simctl install booted bin/01_HelloWorld.app

# Launch app
xcrun simctl launch --console booted io.urho3d.-1-helloworld
```

### Bundle IDs

Apps use format: `io.urho3d.-N-name`
- `01_HelloWorld` → `io.urho3d.-1-helloworld`
- `02_HelloGUI` → `io.urho3d.-2-hellogui`
- `03_Sprites` → `io.urho3d.-3-sprites`

### Simulator Commands

```bash
# List simulators
xcrun simctl list devices available

# Boot specific simulator
xcrun simctl boot "iPhone 16e"

# Check running simulators
xcrun simctl list devices | grep Booted

# Uninstall app
xcrun simctl uninstall booted io.urho3d.-1-helloworld
```

## iOS Device (Requires Apple Developer)

### Setup Code Signing

**Method 1: Environment Variables** (Command Line)

```bash
# From repo root
cp .build-env.template .build-env
```

Edit `.build-env` and add:
```bash
export IOS_DEVELOPMENT_TEAM="YOUR_TEAM_ID"  # Find at developer.apple.com/account
export IOS_CODE_SIGN_IDENTITY="Apple Development"
```

Load and build:
```bash
source .build-env
export PLATFORM=iOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
rake cmake
rake build
```

**Method 2: Xcode GUI** (Recommended)

```bash
# Generate project
export PLATFORM=iOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
rake cmake

# Open in Xcode
open build/ios/Urho3D.xcodeproj
```

In Xcode:
1. Select a target (e.g., `01_HelloWorld`)
2. Go to "Signing & Capabilities"
3. Check "Automatically manage signing"
4. Select your Team
5. Connect your iOS device
6. Click Run (⌘R)

## Build Configuration

### Recommended Settings

```bash
# From repo root
export PLATFORM=iOS
export HOST=macOS
export LIB_TYPE=static
export GENERATOR=xcode

# Disable scripting (has build issues)
export URHO3D_ANGELSCRIPT=0
export URHO3D_LUA=0

# For simulator only
export IOS_SIMULATOR_ONLY=1
```

### Build Types

```bash
# Release (default, optimized)
export CMAKE_BUILD_TYPE=Release

# Debug (with symbols)
export CMAKE_BUILD_TYPE=Debug
```

## Troubleshooting

### "Signing requires a development team"

**For Simulator**: Use `IOS_SIMULATOR_ONLY=1`
```bash
export IOS_SIMULATOR_ONLY=1
rake cmake
```

**For Device**: Set your team ID
```bash
export IOS_DEVELOPMENT_TEAM="YOUR_TEAM_ID"
rake cmake
```

### "Unable to boot simulator"

```bash
# Kill all simulators
killall Simulator

# Boot fresh
xcrun simctl boot "iPhone 16e"
```

### "App crashes on launch"

Check console output:
```bash
xcrun simctl launch --console booted io.urho3d.-1-helloworld
```

**Ignore these warnings** (they're harmless):
- "UIApplicationSupportsIndirectInputEvents"
- "Unbalanced calls to begin/end appearance transitions"

### "No profiles found"

You selected iOS Device instead of Simulator. Change destination in Xcode to "Any iOS Simulator".

### Build errors

If you get compilation errors:
```bash
# Clean and rebuild
rm -rf build/ios
export IOS_SIMULATOR_ONLY=1  # or set IOS_DEVELOPMENT_TEAM
rake cmake
rake build
```

## All 54 Samples Work!

Try any sample:
```bash
cd build/ios
xcrun simctl install booted bin/06_SkeletalAnimation.app
xcrun simctl launch booted io.urho3d.-6-skeletalanimation
```

## See Also

- **[../APPLE_README.md](../APPLE_README.md)** - Apple platforms overview
- **[BuildingForMacOS.md](BuildingForMacOS.md)** - macOS build guide

---

**Last Updated**: 2025-11-06
