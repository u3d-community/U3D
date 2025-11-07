# Building U3D for Apple Platforms

Quick overview for building U3D on iOS and macOS.

## Platform Support

| Platform | Signing | Status | Details |
|----------|---------|--------|---------|
| **macOS** | Not required | ✅ Working | [macOS Guide](Docs/BuildingForMacOS.md) |
| **iOS Simulator** | Ad-hoc (automatic) | ✅ Working | [iOS Guide](Docs/BuildingForIOS.md) |
| **iOS Device** | Apple Developer ID | ✅ Working | [iOS Guide](Docs/BuildingForIOS.md) |

## Quick Start

### macOS
```bash
# From repo root
export PLATFORM=macOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
export URHO3D_ANGELSCRIPT=0 URHO3D_LUA=0 URHO3D_OPENGL=1
export CMAKE_OSX_ARCHITECTURES=arm64
rake cmake
cd build/macos
xcodebuild -target 01_HelloWorld -configuration Release CODE_SIGN_IDENTITY="-"
./bin/01_HelloWorld
```

**→ See [Docs/BuildingForMacOS.md](Docs/BuildingForMacOS.md) for details**

### iOS Simulator
```bash
# From repo root
export PLATFORM=iOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
export IOS_SIMULATOR_ONLY=1
rake cmake && rake build
cd build/ios
xcrun simctl install booted bin/01_HelloWorld.app
xcrun simctl launch --console booted io.urho3d.-1-helloworld
```

**→ See [Docs/BuildingForIOS.md](Docs/BuildingForIOS.md) for details**

### iOS Device
```bash
# From repo root
export IOS_DEVELOPMENT_TEAM="YOUR_TEAM_ID"
export PLATFORM=iOS HOST=macOS LIB_TYPE=static GENERATOR=xcode
rake cmake && rake build
open build/ios/Urho3D.xcodeproj
```

**→ See [Docs/BuildingForIOS.md](Docs/BuildingForIOS.md) for details**

## Prerequisites

- **macOS** 10.15 or later
- **Xcode** (latest version)
- **CMake** 3.10.2+: `brew install cmake`
- **Apple Developer Account** (only for iOS device builds)

## Key Fixes Applied

### 1. FreeType Byte Type (macOS/iOS)
**Fixed**: Modern macOS/iOS now correctly defines `Byte` type in FreeType's zlib code.
- **Files**: `Source/ThirdParty/FreeType/src/gzip/{zconf,ftzconf}.h`
- **Change**: `!defined(TARGET_OS_MAC)` → `!defined(__MACTYPES__)`

### 2. OpenGL Headers (Apple Silicon)
**Fixed**: Apple Silicon Macs now use desktop OpenGL (GLEW) instead of GLES2.
- **File**: `Source/Urho3D/Graphics/OpenGL/OGLGraphicsImpl.h`
- **Change**: Exclude `__APPLE__` from ARM GLES2 check

### 3. iOS Simulator Support
**Added**: `IOS_SIMULATOR_ONLY=1` mode for builds without Apple Developer account.
- **File**: `cmake/Modules/UrhoCommon.cmake`
- **Feature**: Automatic ad-hoc signing for simulator

## Current Limitations

- **AngelScript/Lua**: Disabled due to build errors on macOS
  - Use: `export URHO3D_ANGELSCRIPT=0 URHO3D_LUA=0`
- **Architecture**: Build arm64 only to avoid x86_64 issues
  - Use: `export CMAKE_OSX_ARCHITECTURES=arm64`

## Troubleshooting

### "unknown type name 'Byte'"
FreeType headers not fixed. Check `Source/ThirdParty/FreeType/src/gzip/zconf.h:224` has `!defined(__MACTYPES__)`.

### "GLES2/gl2.h file not found"
OpenGL headers not fixed. Check `Source/Urho3D/Graphics/OpenGL/OGLGraphicsImpl.h:35` excludes `__APPLE__`.

### "Signing requires a development team"
**For iOS Simulator**: Use `export IOS_SIMULATOR_ONLY=1`
**For iOS Device**: Set `export IOS_DEVELOPMENT_TEAM="YOUR_TEAM_ID"`
**For macOS**: No signing needed

## Environment Template

```bash
# From repo root
cp .build-env.template .build-env
# Edit .build-env with your settings
source .build-env
rake cmake && rake build
```

## Documentation

- **[Docs/BuildingForMacOS.md](Docs/BuildingForMacOS.md)** - macOS build guide
- **[Docs/BuildingForIOS.md](Docs/BuildingForIOS.md)** - iOS build guide (Simulator & Device)

## Support

- **GitHub Issues**: https://github.com/u3d-community/U3D/issues
- **Discord**: https://discord.gg/httHCqcXGx

---

**Last Updated**: 2025-11-06 | **Platform**: macOS 15 Sequoia, Xcode 16
