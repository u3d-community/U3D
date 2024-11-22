# Modified by Christophe Ville for U3D project.

# Urho3D - commented out in-source tree build prevention as Urho3D supports both out-of-source and in-source tree builds

# MSVC runtime library flags are selected by an abstraction.
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

cmake_minimum_required(VERSION 3.0.0...3.5)
# Urho3D - commented out setting SDL2 as project name as we want SDL sub-library to remain within Urho3D "umbrella" project
#          instead, just enable the required language support and set the extra variables associated with project() command
enable_language (C CXX)
set (SDL2_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (SDL2_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(SDL2_SUBPROJECT OFF)
else()
  set(SDL2_SUBPROJECT ON)
endif()

if (HAIKU)
  set(LINKER_LANGUAGE CXX)
endif()

set(EXTRA_LIBS)
set(EXTRA_LDFLAGS)

set(CMAKE_DEPENDS)
set(PKGCONFIG_DEPENDS)

# This is a virtual "library" that just exists to collect up compiler and
#  linker options that used to be global to this CMake project. When you
#  specify it as part of a real library's target_link_libraries(), that
#  library will also gain all those build options too. This is meant to
#  modularize old calls to the global add_definitions and include_directories,
#  etc. See https://github.com/libsdl-org/SDL/issues/4150
add_library(sdl-build-options INTERFACE)

if(WINDOWS_STORE)
  target_compile_definitions(sdl-build-options INTERFACE "-DSDL_BUILDING_WINRT=1")
  target_compile_options(sdl-build-options INTERFACE "-ZW")
endif()

# CMake 3.0 expands the "if(${A})" in "set(OFF 1);set(A OFF);if(${A})" to "if(1)"
# CMake 3.24+ emits a warning when not set.
unset(OFF)
unset(ON)
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

# !!! FIXME: this should probably do "MACOSX_RPATH ON" as a target property
# !!! FIXME:  for the SDL2 shared library (so you get an
# !!! FIXME:  install_name ("soname") of "@rpath/libSDL-whatever.dylib"
# !!! FIXME:  instead of "/usr/local/lib/libSDL-whatever.dylib"), but I'm
# !!! FIXME:  punting for now and leaving the existing behavior. Until this
# !!! FIXME:  properly resolved, this line silences a warning in CMake 3.0+.
# !!! FIXME:  remove it and this comment entirely once the problem is
# !!! FIXME:  properly resolved.
#cmake_policy(SET CMP0042 OLD)

# Urho3D - commented out GNUInstallDirs module as the corresponding macros are not being used in our modified version
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckIncludeFile)
include(CheckLanguage)
include(CheckSymbolExists)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckStructHasMember)
include(CMakeDependentOption)
include(CMakePushCheckState)

find_package(PkgConfig)

# Urho3D - commented out CMAKE_MODULE_PATH configuration so that it does not mess up with ours; besides below two include() use file signature instead of module signature
include(${SDL2_SOURCE_DIR}/cmake/macros.cmake)
include(${SDL2_SOURCE_DIR}/cmake/sdlchecks.cmake)
include(${SDL2_SOURCE_DIR}/cmake/sdlplatform.cmake)
include(${SDL2_SOURCE_DIR}/cmake/CheckCPUArchitecture.cmake)

# Enable large file support on 32-bit glibc, so that we can access files
# with large inode numbers
check_symbol_exists("__GLIBC__" "stdlib.h" LIBC_IS_GLIBC)
if (LIBC_IS_GLIBC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

# See docs/release_checklist.md
set(SDL_MAJOR_VERSION 2)
set(SDL_MINOR_VERSION 30)
set(SDL_MICRO_VERSION 9)
set(SDL_VERSION "${SDL_MAJOR_VERSION}.${SDL_MINOR_VERSION}.${SDL_MICRO_VERSION}")

# Urho3D - commented out SDL_CMAKE_DEBUG_POSTFIX configuration as our build system does not not expose SDL as a library

# Calculate a libtool-like version number
math(EXPR SDL_BINARY_AGE "${SDL_MINOR_VERSION} * 100 + ${SDL_MICRO_VERSION}")
if(SDL_MINOR_VERSION MATCHES "[02468]$")
    # Stable branch, 2.24.1 -> libSDL2-2.0.so.0.2400.1
    set(SDL_INTERFACE_AGE ${SDL_MICRO_VERSION})
else()
    # Development branch, 2.23.1 -> libSDL2-2.0.so.0.2301.0
    set(SDL_INTERFACE_AGE 0)
endif()

# Increment this if there is an incompatible change - but if that happens,
# we should rename the library from SDL2 to SDL3, at which point this would
# reset to 0 anyway.
set(LT_MAJOR "0")

math(EXPR LT_AGE "${SDL_BINARY_AGE} - ${SDL_INTERFACE_AGE}")
math(EXPR LT_CURRENT "${LT_MAJOR} + ${LT_AGE}")
set(LT_REVISION "${SDL_INTERFACE_AGE}")
# For historical reasons, the library name redundantly includes the major
# version twice: libSDL2-2.0.so.0.
# TODO: in SDL 3, set the OUTPUT_NAME to plain SDL3, which will simplify
# it to libSDL3.so.0
set(LT_RELEASE "2.0")
set(LT_VERSION "${LT_MAJOR}.${LT_AGE}.${LT_REVISION}")

# Urho3D - commented out DYLIB's current_version and compatibility_version variables as we always build SDL as STATIC

# This list holds all generated headers.
# To avoid generating them twice, these are added to a dummy target on which all sdl targets depend.
# set(SDL_GENERATED_HEADERS)

#message(STATUS "${LT_VERSION} :: ${LT_AGE} :: ${LT_REVISION} :: ${LT_CURRENT} :: ${LT_RELEASE}")

check_cpu_architecture(x86 SDL_CPU_X86)
check_cpu_architecture(x64 SDL_CPU_X64)
check_cpu_architecture(arm32 SDL_CPU_ARM32)
check_cpu_architecture(arm64 SDL_CPU_ARM64)
check_cpu_architecture(loongarch64 SDL_CPU_LOONGARCH64)

# Check for 64 or 32 bit
# Urho3D - bug fix - using CMAKE_SIZEOF_VOID_P is not accurate in a situation where a multilib-capable compiler is chosen and user has opted to use non-native ABI; instead using URHO3D_64BIT build option as it also captures the user intention
if(URHO3D_64BIT)
  set(ARCH_64 TRUE)
  set(PROCESSOR_ARCH "x64")
else()
  set(ARCH_64 FALSE)
  set(PROCESSOR_ARCH "x86")
endif()
set(LIBNAME SDL)
if(NOT LIBTYPE)
  set(LIBTYPE SHARED)
endif()

# Get the platform
SDL_DetectCMakePlatform()

# Don't mistake osx for unix
if(UNIX AND NOT ANDROID AND NOT APPLE AND NOT RISCOS)
  set(UNIX_SYS ON)
else()
  set(UNIX_SYS OFF)
endif()

if(UNIX OR APPLE)
  set(UNIX_OR_MAC_SYS ON)
else()
  set(UNIX_OR_MAC_SYS OFF)
endif()

# Emscripten pthreads work, but you need to have a non-pthread fallback build
#  for systems without support. It's not currently enough to not use
#  pthread functions in a pthread-build; it won't start up on unsupported
#  browsers. As such, you have to explicitly enable it on Emscripten builds
#  for the time being. This default with change to ON once this becomes
#  commonly supported in browsers or the Emscripten teams makes a single
#  binary work everywhere.
if (UNIX_OR_MAC_SYS AND NOT EMSCRIPTEN)
  set(SDL_PTHREADS_ENABLED_BY_DEFAULT ON)
else()
  set(SDL_PTHREADS_ENABLED_BY_DEFAULT OFF)
endif()

if(UNIX_SYS OR ANDROID)
  set(SDL_CLOCK_GETTIME_ENABLED_BY_DEFAULT ON)
else()
  set(SDL_CLOCK_GETTIME_ENABLED_BY_DEFAULT OFF)
endif()

# The hidraw support doesn't catch Xbox, PS4 and Nintendo controllers,
#  so we'll just use libusb when it's available. libusb does not support iOS,
#  so we default to yes on iOS.
#  TODO: Windows can support libusb, the hid.c file just depends on Unix APIs
if((WINDOWS AND NOT WINDOWS_STORE) OR IOS OR TVOS OR ANDROID)
  set(HIDAPI_SKIP_LIBUSB TRUE)
else()
  set(HIDAPI_SKIP_LIBUSB FALSE)
endif()

# On the other hand, *BSD specifically uses libusb only, so we make a special
#  case just for them.
if(FREEBSD OR NETBSD OR OPENBSD OR BSDI)
  set(HIDAPI_ONLY_LIBUSB TRUE)
else()
  set(HIDAPI_ONLY_LIBUSB FALSE)
endif()

# Compiler info
if(CMAKE_C_COMPILER_ID MATCHES "Clang|IntelLLVM")
  set(USE_CLANG TRUE)
  set(OPT_DEF_ASM TRUE)
  # Visual Studio 2019 v16.2 added support for Clang/LLVM.
  # Check if a Visual Studio project is being generated with the Clang toolset.
  if(MSVC)
    set(MSVC_CLANG TRUE)
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC)
  set(USE_GCC TRUE)
  set(OPT_DEF_ASM TRUE)
elseif(MSVC_VERSION GREATER 1400) # VisualStudio 8.0+
  set(OPT_DEF_ASM TRUE)
  #set(CMAKE_C_FLAGS "/ZI /WX- /
elseif(CMAKE_C_COMPILER_ID MATCHES "^Intel$")
  set(OPT_DEF_ASM TRUE)
  set(USE_INTELCC TRUE)
elseif(CMAKE_C_COMPILER_ID MATCHES "QCC")
  set(USE_QCC TRUE)
else()
  set(OPT_DEF_ASM FALSE)
endif()

if(USE_GCC OR USE_CLANG OR USE_INTELCC OR USE_QCC)
  set(OPT_DEF_GCC_ATOMICS ON)
endif()

# Default option knobs
if(UNIX OR MINGW OR MSYS OR (USE_CLANG AND NOT WINDOWS) OR VITA OR PSP OR PS2 OR N3DS)
  set(OPT_DEF_LIBC ON)
endif()
if(WINDOWS OR DARWIN OR MACOSX OR IOS OR TVOS)
  set(SDL_SYSTEM_ICONV_DEFAULT OFF)
else()
  set(SDL_SYSTEM_ICONV_DEFAULT ON)
endif()

if(NOT ("$ENV{CFLAGS}" STREQUAL ""))
  if(CMAKE_VERSION VERSION_LESS 3.11.0)
     message(WARNING "SDL's CMakeLists.txt no longer checks the CFLAGS environment.")
     message(WARNING "Please use CMake's CMAKE_C_FLAGS and CMAKE_BUILD_TYPE variables directly.")
     message(WARNING "Or upgrade to CMake >= 3.11.0, which respects the CFLAGS environment var.")
  endif()
endif()

# Build in parallel under Visual Studio. Not enabled by default.
if(MSVC AND NOT USE_CLANG)
  target_compile_options(sdl-build-options INTERFACE "/MP")
endif()

# Urho3D - commented out compiler/linker flags setup as we have configured all our compiler/linker flags as we want globally

# Those are used for pkg-config and friends, so that the sdl2.pc, sdl2-config,
# etc. are created correctly.
# Urho3D - we do not use these variables for pkg-config setup for SDL library but we may need them later for Urho3D library
set(SDL_LIBS )
set(SDL_CFLAGS )

# When building shared lib for Windows with MinGW,
# avoid the DLL having a "lib" prefix
if(WINDOWS)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -D_GNU_SOURCE=1")

# Emscripten toolchain has a nonempty default value for this, and the checks
# in this file need to change that, so remember the original value, and
# restore back to that afterwards. For check_function_exists() to work in
# Emscripten, this value must be at its default value.
# Urho3D - also due to a CMake's try_compile() bug on iOS/tvOS platform, the CMAKE_REQUIRED_FLAGS contains our SmileyHack-injection on iOS/tvOS platform
set(ORIG_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
# Urho3D - save the original CMake global settings, do not leave them to chances
set (ORIG_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set (ORIG_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})

# Urho3D - bug fix - adjust the header search path for subsequent pthread, EGL and OpenGLES2 checks
if (RPI)
  list (APPEND CMAKE_REQUIRED_INCLUDES ${VIDEOCORE_INCLUDE_DIRS})
endif ()

if(CYGWIN)
  # We build SDL on cygwin without the UNIX emulation layer
  target_include_directories(sdl-build-options INTERFACE "/usr/include/mingw")
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -mno-cygwin")
  check_c_source_compiles("int main(int argc, char **argv) { return 0; }"
    HAVE_GCC_NO_CYGWIN)
  set(CMAKE_REQUIRED_FLAGS ${ORIG_CMAKE_REQUIRED_FLAGS})
  if(HAVE_GCC_NO_CYGWIN)
    list(APPEND EXTRA_LDFLAGS_BUILD "-mno-cygwin")
    list(APPEND SDL_LIBS "-mno-cygwin")
  endif()
  list(APPEND SDL_CFLAGS "-I/usr/include/mingw")
endif()

# General includes
# Urho3D - use 'generated' path suffix for generated config header file
target_compile_definitions(sdl-build-options INTERFACE "-DUSING_GENERATED_CONFIG_H")
target_include_directories(sdl-build-options BEFORE INTERFACE ${SDL2_BINARY_DIR}/include/generated "${SDL2_SOURCE_DIR}/include")
# Note: The clang toolset for Visual Studio does not support the '-idirafter' option.
if(USE_GCC OR (USE_CLANG AND NOT MSVC_CLANG))
  # !!! FIXME: do we _need_ to mess with CMAKE_C_FLAGS here?
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -idirafter \"${SDL2_SOURCE_DIR}/src/video/khronos\"")
else()
  target_include_directories(sdl-build-options INTERFACE "${SDL2_SOURCE_DIR}/src/video/khronos")
endif()

# All these ENABLED_BY_DEFAULT vars will default to ON if not specified, so
#  you only need to have a platform override them if they are disabling.
if(EMSCRIPTEN)
  # Set up default values for the currently supported set of subsystems:
  # Emscripten/Javascript does not have assembly support, a dynamic library
  # loading architecture, or low-level CPU inspection.

  # SDL_THREADS_ENABLED_BY_DEFAULT now defaults to ON, but pthread support might be disabled by default.
  # !!! FIXME: most of these subsystems should default to ON if there are dummy implementations to be used.

  set(OPT_DEF_ASM FALSE)
  set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)
  set(SDL_ATOMIC_ENABLED_BY_DEFAULT OFF)
  set(SDL_LOADSO_ENABLED_BY_DEFAULT OFF)
  # Urho3D - enable cpuinfo
  set(SDL_CPUINFO_ENABLED_BY_DEFAULT ON)
  # Urho3D - disable pthreads : some build issues with sdl2.30.9
  set(SDL_THREADS_ENABLED_BY_DEFAULT OFF)
endif()

if(VITA OR PSP OR PS2 OR N3DS)
  set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)
  set(SDL_LOADSO_ENABLED_BY_DEFAULT OFF)
endif()

# Urho3D - by default we build internal sub-libraries as STATIC library type, so default shared build off
set(SDL_STATIC_ENABLED_BY_DEFAULT ON)

set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)

# Urho3D - bug fix - add compiler defined expected by SDL.c when building STATIC library type
if (WINDOWS)
    add_definitions (-DSDL_STATIC_LIB)
endif ()

# Urho3D - we prefer to disable 2D render subsystem by default
if (NOT DEFINED SDL_RENDER_ENABLED_BY_DEFAULT)
    set (SDL_RENDER_ENABLED_BY_DEFAULT OFF)
endif()

# Urho3D - remove test
set(SDL_TEST_ENABLED_BY_DEFAULT OFF)

# Urho3D - LONGESTOPTIONNAME default 0 value to 26
set(LONGESTOPTIONNAME 26)  

set(SDL_SUBSYSTEMS
    Atomic Audio Video Render Events Joystick Haptic Hidapi Power Threads Timers
    File Loadso CPUinfo Filesystem Sensor Locale Misc)
foreach(_SUB ${SDL_SUBSYSTEMS})
  string(TOUPPER ${_SUB} _OPT)
  if (NOT DEFINED SDL_${_OPT}_ENABLED_BY_DEFAULT)
    set(SDL_${_OPT}_ENABLED_BY_DEFAULT ON)
  endif()
  option(SDL_${_OPT} "Enable the ${_SUB} subsystem" ${SDL_${_OPT}_ENABLED_BY_DEFAULT})
endforeach()

# Urho3D - Disable Sdlmain, install, uninstall, test
set(SDL2_DISABLE_SDL2MAIN ON)
set(SDL2_DISABLE_INSTALL ON)
set(SDL2_DISABLE_UNINSTALL ON)
set(SDL_TEST OFF)
set(SDL_TESTS OFF)
set(SDL_INSTALL_TESTS OFF)

option_string(SDL_ASSERTIONS "Enable internal sanity checks (auto/disabled/release/enabled/paranoid)" "auto")
#set_option(SDL_DEPENDENCY_TRACKING "Use gcc -MMD -MT dependency tracking" ON)
set_option(SDL_ASSEMBLY            "Enable assembly routines" ${OPT_DEF_ASM})
# Urho3D - commented out SSEMATH, SSE, SSE2, SSE3, MMX, 3DNOW, ALTIVEC build options as they have been reclassifified as Urho3D own build options
dep_option(SDL_ARMSIMD             "Use SIMD assembly blitters on ARM" OFF "SDL_ASSEMBLY;SDL_CPU_ARM32" OFF)
dep_option(SDL_ARMNEON             "Use NEON assembly blitters on ARM" OFF "SDL_ASSEMBLY;SDL_CPU_ARM32" OFF)
dep_option(SDL_LSX                 "Use LSX assembly routines" ON "SDL_ASSEMBLY;SDL_CPU_LOONGARCH64" OFF)
dep_option(SDL_LASX                "Use LASX assembly routines" ON "SDL_ASSEMBLY;SDL_CPU_LOONGARCH64" OFF)

set_option(SDL_LIBC                "Use the system C library" ${OPT_DEF_LIBC})
# Urho3D - bug fix - also enable GCC_ATOMICS by default for Clang (including Emscripten which is a Clang derivative)
dep_option(SDL_GCC_ATOMICS         "Use gcc builtin atomics" ON "USE_GCC OR USE_CLANG" OFF)
dep_option(SDL_DBUS                "Enable D-Bus support" ON ${UNIX_SYS} OFF)
set_option(SDL_DISKAUDIO           "Support the disk writer audio driver" ON)
set_option(SDL_DUMMYAUDIO          "Support the dummy audio driver" ON)
set_option(SDL_DIRECTFB            "Use DirectFB video driver" OFF)
dep_option(SDL_DIRECTFB_SHARED     "Dynamically load directfb support" ON "SDL_DIRECTFB" OFF)
set_option(SDL_DUMMYVIDEO          "Use dummy video driver" ON)
dep_option(SDL_IBUS                "Enable IBus support" ON ${UNIX_SYS} OFF)
set_option(SDL_SYSTEM_ICONV        "Use iconv() from system-installed libraries" ${SDL_SYSTEM_ICONV_DEFAULT})
set_option(SDL_LIBICONV            "Prefer iconv() from libiconv, if available, over libc version" OFF)
# Urho3D - hide OpenGL option from all non-desktop platforms
dep_option(SDL_OPENGL              "Include OpenGL support" ON "NOT ANDROID AND NOT IOS AND NOT TVOS AND NOT ARM AND NOT WEB" OFF)
# Urho3D - hide OpenGLES option from all desktop platforms, although a few of them appears to have the necessary header file to build SDL OpenGLES video driver
dep_option(SDL_OPENGLES            "Include OpenGL ES support" ON "ANDROID OR IOS OR TVOS OR ARM OR WEB" OFF)
set_option(SDL_PTHREADS            "Use POSIX threads for multi-threading" ${SDL_THREADS_ENABLED_BY_DEFAULT})
dep_option(SDL_PTHREADS_SEM        "Use pthread semaphores" ON "SDL_PTHREADS" OFF)
# Urho3D - SDL_DLOPEN was originally set as option twice (one here and one in the subsystem section above), we keep the one here so that we can make the option dependent on other conditions
dep_option2(SDL_DLOPEN              "Use dlopen for shared object loading" ON "UNIX\;NOT IOS OR NOT IPHONEOS_DEPLOYMENT_TARGET OR NOT IPHONEOS_DEPLOYMENT_TARGET VERSION_LESS 8" OFF)    # No need to check for tvOS as its target version is 9 or above
dep_option(SDL_OSS                 "Support the OSS audio API" ON "UNIX_SYS OR RISCOS" OFF)
set_option(SDL_ALSA                "Support the ALSA audio API" ${UNIX_SYS})
dep_option(SDL_ALSA_SHARED         "Dynamically load ALSA audio support" ON "SDL_ALSA" OFF)
set_option(SDL_JACK                "Support the JACK audio API" ${UNIX_SYS})
dep_option(SDL_JACK_SHARED         "Dynamically load JACK audio support" ON "SDL_JACK" OFF)
set_option(SDL_ESD                 "Support the Enlightened Sound Daemon" ${UNIX_SYS})
dep_option(SDL_ESD_SHARED          "Dynamically load ESD audio support" ON "SDL_ESD" OFF)
set_option(SDL_PIPEWIRE            "Use Pipewire audio" ${UNIX_SYS})
dep_option(SDL_PIPEWIRE_SHARED     "Dynamically load Pipewire support" ON "SDL_PIPEWIRE" OFF)
set_option(SDL_PULSEAUDIO          "Use PulseAudio" ${UNIX_SYS})
dep_option(SDL_PULSEAUDIO_SHARED   "Dynamically load PulseAudio support" ON "SDL_PULSEAUDIO" OFF)
set_option(SDL_ARTS                "Support the Analog Real Time Synthesizer" ${UNIX_SYS})
dep_option(SDL_ARTS_SHARED         "Dynamically load aRts audio support" ON "SDL_ARTS" OFF)
set_option(SDL_NAS                 "Support the NAS audio API" ${UNIX_SYS})
dep_option(SDL_NAS_SHARED          "Dynamically load NAS audio support" ON "SDL_NAS" OFF)
set_option(SDL_SNDIO               "Support the sndio audio API" ${UNIX_SYS})
dep_option(SDL_SNDIO_SHARED        "Dynamically load the sndio audio API" ON "SDL_SNDIO" OFF)
set_option(SDL_FUSIONSOUND         "Use FusionSound audio driver" OFF)
dep_option(SDL_FUSIONSOUND_SHARED  "Dynamically load fusionsound audio support" ON "SDL_FUSIONSOUND" OFF)
set_option(SDL_LIBSAMPLERATE       "Use libsamplerate for audio rate conversion" ${UNIX_SYS})
dep_option(SDL_LIBSAMPLERATE_SHARED "Dynamically load libsamplerate" ON "SDL_LIBSAMPLERATE" OFF)
# Urho3D - commented out SDL_RPATH as an option because Urho3D is configured to always use RPATH, so init the variable to always TRUE
set (SDL_RPATH TRUE)
set_option(SDL_CLOCK_GETTIME       "Use clock_gettime() instead of gettimeofday()" ${SDL_CLOCK_GETTIME_ENABLED_BY_DEFAULT})
# Urho3D - bug fix - make VIDEO_X11 build option available for desktop UNIX (including OSX) only
dep_option2(SDL_X11                 "Use X11 video driver" ON "UNIX_SYS OR APPLE\;NOT RPI AND NOT IOS AND NOT TVOS" OFF)
dep_option(SDL_X11_SHARED          "Dynamically load X11 support" ON "SDL_X11" OFF)
set(SDL_X11_OPTIONS Xcursor Xdbe XInput Xfixes Xrandr Xscrnsaver XShape)
foreach(_SUB ${SDL_X11_OPTIONS})
  string(TOUPPER "SDL_X11_${_SUB}" _OPT)
  dep_option(${_OPT}               "Enable ${_SUB} support" ON "SDL_X11" OFF)
endforeach()
set_option(SDL_WAYLAND             "Use Wayland video driver" ${UNIX_SYS})
dep_option(SDL_WAYLAND_SHARED      "Dynamically load Wayland support" ON "SDL_WAYLAND" OFF)
dep_option(SDL_WAYLAND_LIBDECOR    "Use client-side window decorations on Wayland" ON "SDL_WAYLAND" OFF)
dep_option(SDL_WAYLAND_LIBDECOR_SHARED     "Dynamically load libdecor support" ON "SDL_WAYLAND_LIBDECOR;SDL_WAYLAND_SHARED" OFF)
dep_option(SDL_WAYLAND_QT_TOUCH    "QtWayland server support for Wayland video driver" ON "SDL_WAYLAND" OFF)
# Urho3D - only make VIDEO_RPI option available on Raspberry-Pi platform
dep_option(SDL_RPI                 "Use Raspberry Pi video driver" ON "RPI AND NOT RPI_ABI STREQUAL RPI4" OFF)
# Urho3D - bug fix - only make the COCOA option available on OSX platform but not iOS/tvOS; it does not make too much sense now for user to disable it because SDL/Apple has dropped support for XQuartz (X11 on Mac), so Cocoa is the only video driver available
#          But we still keep it as an option here because it should be possible to add back the XQuartz support
dep_option(SDL_COCOA               "Use Cocoa video driver" ON "APPLE AND NOT IOS AND NOT TVOS" OFF)
# Urho3D - only make DIRECTX option available on Windows platform when URHO3D_OPENGL is enabled, i.e. DIRECTX variable must always be ON (not an option) when URHO3D_OPENGL is disabled because in this case it must depend on Direct3D which is a component of DirectX
if (WINDOWS)
  dep_option(SDL_DIRECTX           "Use DirectX for Windows audio/video" ON URHO3D_OPENGL ON)
else ()
  set (SDL_DIRECTX OFF)  # Make it off explicitly when not targeting Windows platform, just in case user passes it in accidentally via CLI
endif ()
set_option(SDL_XINPUT              "Use Xinput for Windows" ${WINDOWS})
set_option(SDL_WASAPI              "Use the Windows WASAPI audio driver" ${WINDOWS})
# Urho3D - commented out RENDER_D3D as an option to avoid potential conflict with our URHO3D_OPENGL and URHO3D_D3D11 build options on Windows platform
#          Instead just initialize the variable according to our build options; Urho3D also by default disables the SDL renderer subsystem
if (WINDOWS)
  if (URHO3D_OPENGL)
    set (SDL_RENDER_D3D FALSE)
  else ()
    set (SDL_RENDER_D3D TRUE)
  endif ()
endif ()
set_option(SDL_RENDER_METAL        "Enable the Metal render driver" ${APPLE})
set_option(SDL_VIVANTE             "Use Vivante EGL video driver" ${UNIX_SYS})
dep_option(SDL_VULKAN              "Enable Vulkan support" ON "ANDROID OR APPLE OR LINUX OR WINDOWS" OFF)
set_option(SDL_METAL               "Enable Metal support" ${APPLE})
set_option(SDL_KMSDRM              "Use KMS DRM video driver" ${UNIX_SYS})
dep_option(SDL_KMSDRM_SHARED       "Dynamically load KMS DRM support" ON "SDL_KMSDRM" OFF)
# Urho3D - default Offscreen to off
set_option(SDL_OFFSCREEN           "Use offscreen video driver" OFF)
option_string(SDL_BACKGROUNDING_SIGNAL "number to use for magic backgrounding signal or 'OFF'" OFF)
option_string(SDL_FOREGROUNDING_SIGNAL "number to use for magic foregrounding signal or 'OFF'" OFF)
set_option(SDL_HIDAPI              "Enable the HIDAPI subsystem" ON)
dep_option(SDL_HIDAPI_LIBUSB       "Use libusb for low level joystick drivers" OFF SDL_HIDAPI OFF)
dep_option(SDL_HIDAPI_JOYSTICK     "Use HIDAPI for low level joystick drivers" ON SDL_HIDAPI OFF)
dep_option(SDL_VIRTUAL_JOYSTICK    "Enable the virtual-joystick driver" ON SDL_HIDAPI OFF)
set_option(SDL_LIBUDEV             "Enable libudev support" ON)
set_option(SDL_ASAN                "Use AddressSanitizer to detect memory errors" OFF)
option_string(SDL_VENDOR_INFO      "Vendor name and/or version to add to SDL_REVISION" "")
set_option(SDL_CCACHE              "Use Ccache to speed up build" ON)

option(SDL_WERROR "Enable -Werror" OFF)

option(SDL_SHARED "Build a shared version of the library" ${SDL_SHARED_ENABLED_BY_DEFAULT})
option(SDL_STATIC "Build a static version of the library" ${SDL_STATIC_ENABLED_BY_DEFAULT})
# Urho3D - remove option to build the test directory

dep_option(SDL_STATIC_PIC      "Static version of the library should be built with Position Independent Code" "${CMAKE_POSITION_INDEPENDENT_CODE}" "SDL_STATIC" OFF)

set(HAVE_STATIC_PIC "${SDL_STATIC_PIC}")

if(SDL_HIDAPI)
  if(HIDAPI_ONLY_LIBUSB)
    set(SDL_HIDAPI_LIBUSB ON CACHE BOOL "" FORCE)
  elseif(HIDAPI_SKIP_LIBUSB)
    set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "" FORCE)
  endif()
endif()

if(VITA)
  set_option(VIDEO_VITA_PIB  "Build with PSVita piglet gles2 support" OFF)
  set_option(VIDEO_VITA_PVR  "Build with PSVita PVR gles/gles2 support" OFF)
endif()

# Urho3D - remove sources for all disabled systems
# General source files
set (SOURCE_FILE_PATTERNS
  ${SDL2_SOURCE_DIR}/src/*.c
  ${SDL2_SOURCE_DIR}/src/atomic/*.c
# Urho3D - we always disable dynamic API, so commented out ${SDL2_SOURCE_DIR}/src/dynapi/*.c
  ${SDL2_SOURCE_DIR}/src/libm/*.c
  ${SDL2_SOURCE_DIR}/src/stdlib/*.c
  ${SDL2_SOURCE_DIR}/src/thread/*.c
  ${SDL2_SOURCE_DIR}/src/timer/*.c)
# Urho3D - exclude source files from disabled subsystems, except SDL_ATOMIC, SDL_THREADS and SDL_TIMERS as SDL always needs them even when they may be just generic or dummy implementations
foreach (_SUB AUDIO CPUINFO EVENTS FILE HAPTIC HIDAPI JOYSTICK LOCALE MISC POWER RENDER SENSOR VIDEO)
  string (TOLOWER ${_SUB} _DIR)
  if (${SDL_${_SUB}})
    list (APPEND SOURCE_FILE_PATTERNS ${SDL2_SOURCE_DIR}/src/${_DIR}/*.c)
  endif ()
endforeach ()
if (SDL_RENDER)
  list (APPEND SOURCE_FILE_PATTERNS ${SDL2_SOURCE_DIR}/src/render/*/*.c)   # Recurse one more level
endif ()
if (SDL_VIDEO)
  list (APPEND SOURCE_FILE_PATTERNS ${SDL2_SOURCE_DIR}/src/video/yuv2rgb/*.c)
endif ()
file (GLOB SOURCE_FILES ${SOURCE_FILE_PATTERNS})

if(USE_INTELCC)
  # warning #39: division by zero
  # warning #239: floating point underflow
  # warning #264: floating-point value does not fit in required floating-point type
  set_property(SOURCE "${SDL2_SOURCE_DIR}/src/libm/e_exp.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -wd239 -wd264")
  set_property(SOURCE "${SDL2_SOURCE_DIR}/src/libm/e_log.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -wd39")
  set_property(SOURCE "${SDL2_SOURCE_DIR}/src/libm/e_log10.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -wd39")
endif()


set(SDL_DEFAULT_ASSERT_LEVEL_CONFIGURED 1)
if(SDL_ASSERTIONS MATCHES "^(auto|)$")
  # Do nada - use optimization settings to determine the assertion level
  set(SDL_DEFAULT_ASSERT_LEVEL )
  set(SDL_DEFAULT_ASSERT_LEVEL_CONFIGURED 0)
elseif(SDL_ASSERTIONS MATCHES "^(disabled|0)$")
  set(SDL_DEFAULT_ASSERT_LEVEL 0)
elseif(SDL_ASSERTIONS MATCHES "^(release|1)$")
  set(SDL_DEFAULT_ASSERT_LEVEL 1)
elseif(SDL_ASSERTIONS MATCHES "^(enabled|2)$")
  set(SDL_DEFAULT_ASSERT_LEVEL 2)
elseif(SDL_ASSERTIONS MATCHES "^(paranoid|3)$")
  set(SDL_DEFAULT_ASSERT_LEVEL 3)
else()
  message_error("unknown assertion level")
endif()
set(HAVE_ASSERTIONS ${SDL_ASSERTIONS})

if(NOT SDL_BACKGROUNDING_SIGNAL STREQUAL "OFF")
  target_compile_definitions(sdl-build-options INTERFACE "-DSDL_BACKGROUNDING_SIGNAL=${SDL_BACKGROUNDING_SIGNAL}")
endif()

if(NOT SDL_FOREGROUNDING_SIGNAL STREQUAL "OFF")
  target_compile_definitions(sdl-build-options INTERFACE "-DSDL_FOREGROUNDING_SIGNAL=${SDL_FOREGROUNDING_SIGNAL}")
endif()

# Compiler option evaluation
if(USE_GCC OR USE_CLANG OR USE_INTELCC OR USE_QCC)
  # Check for -Wall first, so later things can override pieces of it.
  # Note: clang-cl treats -Wall as -Weverything (which is very loud),
  #       /W3 as -Wall, and /W4 as -Wall -Wextra.  So: /W3 is enough.
  check_c_compiler_flag(-Wall HAVE_GCC_WALL)
  if(MSVC_CLANG)
    list(APPEND EXTRA_CFLAGS "/W3")
  elseif(HAVE_GCC_WALL)
    list(APPEND EXTRA_CFLAGS "-Wall")
    if(HAIKU)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-multichar")
    endif()
  endif()

  check_c_compiler_flag(-Wundef HAVE_GCC_WUNDEF)
  if(HAVE_GCC_WUNDEF)
    list(APPEND EXTRA_CFLAGS "-Wundef")
  endif()

  check_c_compiler_flag(-fno-strict-aliasing HAVE_GCC_NO_STRICT_ALIASING)
  if(HAVE_GCC_NO_STRICT_ALIASING)
    list(APPEND EXTRA_CFLAGS "-fno-strict-aliasing")
  endif()

  check_c_compiler_flag(-Wdocumentation HAVE_GCC_WDOCUMENTATION)
  if(HAVE_GCC_WDOCUMENTATION)
    if(SDL_WERROR)
      check_c_compiler_flag(-Werror=documentation HAVE_GCC_WERROR_DOCUMENTATION)
      if(HAVE_GCC_WERROR_DOCUMENTATION)
        list(APPEND EXTRA_CFLAGS "-Werror=documentation")
      endif()
    endif()
     list(APPEND EXTRA_CFLAGS "-Wdocumentation")
  endif()

  check_c_compiler_flag(-Wdocumentation-unknown-command HAVE_GCC_WDOCUMENTATION_UNKNOWN_COMMAND)
  if(HAVE_GCC_WDOCUMENTATION_UNKNOWN_COMMAND)
    if(SDL_WERROR)
      check_c_compiler_flag(-Werror=documentation-unknown-command HAVE_GCC_WERROR_DOCUMENTATION_UNKNOWN_COMMAND)
      if(HAVE_GCC_WERROR_DOCUMENTATION_UNKNOWN_COMMAND)
        list(APPEND EXTRA_CFLAGS "-Werror=documentation-unknown-command")
      endif()
    endif()
    list(APPEND EXTRA_CFLAGS "-Wdocumentation-unknown-command")
  endif()

  check_c_compiler_flag(-fcomment-block-commands=threadsafety HAVE_GCC_COMMENT_BLOCK_COMMANDS)
  if(HAVE_GCC_COMMENT_BLOCK_COMMANDS)
    list(APPEND EXTRA_CFLAGS "-fcomment-block-commands=threadsafety")
    list(APPEND EXTRA_CFLAGS "-fcomment-block-commands=deprecated")
  else()
    check_c_compiler_flag(/clang:-fcomment-block-commands=threadsafety HAVE_CLANG_COMMENT_BLOCK_COMMANDS)
    if(HAVE_CLANG_COMMENT_BLOCK_COMMANDS)
      list(APPEND EXTRA_CFLAGS "/clang:-fcomment-block-commands=threadsafety")
      list(APPEND EXTRA_CFLAGS "/clang:-fcomment-block-commands=deprecated")
    endif()
  endif()

  # Urho3D - Remove -Wdeclaration-after-statement (with this flag, issue in hid code ; don't need to check old C < std99)

  if(DEPENDENCY_TRACKING)
    check_c_source_compiles("
        #if !defined(__GNUC__) || __GNUC__ < 3
        #error Dependency tracking requires GCC 3.0 or newer
        #endif
        int main(int argc, char **argv) { return 0; }" HAVE_DEPENDENCY_TRACKING)
  endif()

  if(SDL_GCC_ATOMICS)
    check_c_source_compiles("int main(int argc, char **argv) {
        int a;
        void *x, *y, *z;
        __sync_lock_test_and_set(&a, 4);
        __sync_lock_test_and_set(&x, y);
        __sync_fetch_and_add(&a, 1);
        __sync_bool_compare_and_swap(&a, 5, 10);
        __sync_bool_compare_and_swap(&x, y, z);
        return 0; }" HAVE_GCC_ATOMICS)
    if(NOT HAVE_GCC_ATOMICS)
      check_c_source_compiles("int main(int argc, char **argv) {
          int a;
          __sync_lock_test_and_set(&a, 1);
          __sync_lock_release(&a);
          return 0; }" HAVE_GCC_SYNC_LOCK_TEST_AND_SET)
    endif()
  endif()

  # Urho3D - we rely on GenerateExportHeader CMake module for configuring GCC/Clang visibility attribute support

  check_c_compiler_flag(-Wshadow HAVE_GCC_WSHADOW)
  if(HAVE_GCC_WSHADOW)
    list(APPEND EXTRA_CFLAGS "-Wshadow")
  endif()

  check_c_compiler_flag(-Wunused-local-typedefs HAVE_GCC_WUNUSED_LOCAL_TYPEDEFS)
  if(HAVE_GCC_WUNUSED_LOCAL_TYPEDEFS)
    list(APPEND EXTRA_CFLAGS "-Wno-unused-local-typedefs")
  endif()

  # Urho3D - there is no need to use AppleClang-specific linker flags, also because we have fixed our CMAKE_REQUIRED_FLAGS to make the detection reliable

  if(MINGW)
    # See if GCC's -gdwarf-4 is supported
    # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101377 for why this is needed on Windows
    cmake_push_check_state(RESET)
    check_c_compiler_flag("-gdwarf-4" HAVE_GDWARF_4)
    if(HAVE_GDWARF_4)
      set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -gdwarf-4")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-4")
    endif()
    cmake_pop_check_state()
  endif()

  # Force color diagnostics when one of these conditions are met
  if(DEFINED ENV{CI} OR DEFINED ENV{USE_CCACHE} OR CMAKE_GENERATOR MATCHES Ninja)
    if(EMSCRIPTEN OR (USE_GCC AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.9))
      list(APPEND EXTRA_CFLAGS -fdiagnostics-color=always)
    elseif(USE_CLANG AND NOT CMAKE_C_COMPILER_ID MATCHES AppleClang)
      list(APPEND EXTRA_CFLAGS -fcolor-diagnostics)
    endif()
  endif()

  # Urho3D - Wayland quick fix
  list(APPEND EXTRA_CFLAGS "-fcommon")
endif()
if(MSVC)
  target_compile_definitions(sdl-build-options INTERFACE "-D_CRT_SECURE_NO_DEPRECATE")
  target_compile_definitions(sdl-build-options INTERFACE "-D_CRT_NONSTDC_NO_DEPRECATE")
  target_compile_definitions(sdl-build-options INTERFACE "-D_CRT_SECURE_NO_WARNINGS")

  # CET support was added in VS 16.7
  if(MSVC_VERSION GREATER 1926 AND CMAKE_GENERATOR_PLATFORM MATCHES "Win32|x64")
    list(APPEND EXTRA_LDFLAGS_BUILD "-CETCOMPAT")
  endif()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # Due to a limitation of Microsoft's LTO implementation, LTO must be disabled for memcpy and memset.
  # The same applies to various functions normally belonging in the C library (for x86 architecture).
  set_property(SOURCE src/stdlib/SDL_mslibc.c APPEND_STRING PROPERTY COMPILE_FLAGS " /GL-")
endif()

if(SDL_ASSEMBLY)
  if(USE_GCC OR USE_CLANG OR USE_INTELCC)
    # TODO: Those all seem to be quite GCC specific - needs to be
    # reworked for better compiler support
    set(HAVE_ASSEMBLY TRUE)

    # Urho3D - move the altivec, mmx, 3dnow, sse, sse2, and sse3 checks to Urho3D common module which then configure the compiler flags globally

    # Urho3D - bug fix - do not use "-mfpmath=387" in EXTRA_CFLAGS, instead let the compiler to choose what's the best by itself (i.e. '387' for x86 and 'sse' for x86_64) and also to keep SDL library being built with the same option as Urho3D library and all the other 3rd-party sub-libraries; besides the '387' is invalid when targeting arm64

    # Urho3D - commented out the HAVE_SSEMATH variable as it is not being used anywhere currently; furthermore compiler already emits __SSE_MATH__ or __SSE2_MATH__ as necessary on x86_64 ABI only; so instead of using HAVE_SSEMATH variable, we should actually use the __SSE_MATH__ or __SSE2_MATH__ compiler define if that is the original intention of having this variable

    check_include_file("immintrin.h" HAVE_IMMINTRIN_H)

    if(SDL_ALTIVEC)
      set(CMAKE_REQUIRED_FLAGS "-maltivec")
      check_c_source_compiles("
          #include <altivec.h>
          vector unsigned int vzero() {
              return vec_splat_u32(0);
          }
          int main(int argc, char **argv) { return 0; }" HAVE_ALTIVEC_H_HDR)
      check_c_source_compiles("
          vector unsigned int vzero() {
              return vec_splat_u32(0);
          }
          int main(int argc, char **argv) { return 0; }" CPU_SUPPORTS_ALTIVEC)
      set(CMAKE_REQUIRED_FLAGS ${ORIG_CMAKE_REQUIRED_FLAGS})
      if(CPU_SUPPORTS_ALTIVEC OR HAVE_ALTIVEC_H_HDR)
        set(HAVE_ALTIVEC TRUE) # if only HAVE_ALTIVEC_H_HDR is set
        list(APPEND EXTRA_CFLAGS "-maltivec")
        set(SDL_ALTIVEC_BLITTERS 1)
        if(HAVE_ALTIVEC_H_HDR)
          set(HAVE_ALTIVEC_H 1)
        endif()
      endif()
    endif()

    if(SDL_LSX)
      cmake_push_check_state()
      set(CMAKE_REQUIRED_FLAGS "-mlsx")
      check_c_source_compiles("
          #ifndef __loongarch_sx
          #error Assembler CPP flag not enabled
          #endif
          int main(int argc, char **argv) { return 0; }" CPU_SUPPORTS_LSX)
      check_include_file("lsxintrin.h" HAVE_LSXINTRIN_H)
      cmake_pop_check_state()

      if(CPU_SUPPORTS_LSX AND HAVE_LSXINTRIN_H)
        list(APPEND EXTRA_CFLAGS "-mlsx")
        set(HAVE_LSX TRUE)
      endif()
    endif()

    if(SDL_ARMSIMD)
      # Urho3D - remove copy CMAKE_REQUIRED_FLAGS in ORIG_CMAKE_REQUIRED_FLAGS
      set(CMAKE_REQUIRED_FLAGS "${ORIG_CMAKE_REQUIRED_FLAGS} -x assembler-with-cpp")
      check_c_source_compiles("
        .text
        .arch armv6
        .object_arch armv4
        .arm
        .altmacro
        #ifndef __ARM_EABI__
        #error EABI is required (to be sure that calling conventions are compatible)
        #endif
        pld [r0]
        uqadd8 r0, r0, r0
      " ARMSIMD_FOUND)
      set(CMAKE_REQUIRED_FLAGS "${ORIG_CMAKE_REQUIRED_FLAGS}")

      if(ARMSIMD_FOUND)
        set(HAVE_ARMSIMD TRUE)
        set(SDL_ARM_SIMD_BLITTERS 1)
        file(GLOB ARMSIMD_SOURCES ${SDL2_SOURCE_DIR}/src/video/arm/pixman-arm-simd*.S)
        list(APPEND SOURCE_FILES ${ARMSIMD_SOURCES})
        set(WARN_ABOUT_ARM_SIMD_ASM_MIT TRUE)
      endif()
    endif()

    if(SDL_ARMNEON)
      # Urho3D - remove copy CMAKE_REQUIRED_FLAGS in ORIG_CMAKE_REQUIRED_FLAGS
      set(CMAKE_REQUIRED_FLAGS "${ORIG_CMAKE_REQUIRED_FLAGS} -x assembler-with-cpp")
      check_c_source_compiles("
        .text
        .fpu neon
        .arch armv7a
        .object_arch armv4
        .eabi_attribute 10, 0
        .arm
        .altmacro
        #ifndef __ARM_EABI__
        #error EABI is required (to be sure that calling conventions are compatible)
        #endif
        pld [r0]
        vmovn.u16 d0, q0
      " ARMNEON_FOUND)
      set(CMAKE_REQUIRED_FLAGS "${ORIG_CMAKE_REQUIRED_FLAGS}")

      if(ARMNEON_FOUND)
        set(HAVE_ARMNEON TRUE)
        set(SDL_ARM_NEON_BLITTERS 1)
        file(GLOB ARMNEON_SOURCES ${SDL2_SOURCE_DIR}/src/video/arm/pixman-arm-neon*.S)
        list(APPEND SOURCE_FILES ${ARMNEON_SOURCES})
        set(WARN_ABOUT_ARM_NEON_ASM_MIT TRUE)
      endif()
    endif()

  elseif(MSVC_VERSION GREATER 1500)
    # TODO: SDL_cpuinfo.h needs to support the user's configuration wish
    # Urho3D - move the HAVE_MMX, HAVE_3DNOW, HAVE_SSE, HAVE_SSE2, and HAVE_SSE3 initialization to Urho3D common module
    set(SDL_ASSEMBLY_ROUTINES 1)
  endif()
endif()

# TODO: Can't deactivate on FreeBSD? w/o LIBC, SDL_stdinc.h can't define
# anything.
if(SDL_LIBC)
  if(WINDOWS AND NOT MINGW)
    set(HAVE_LIBC TRUE)
    check_include_file(stdint.h HAVE_STDINT_H)
    foreach(_HEADER stdio.h string.h wchar.h ctype.h math.h limits.h)
      string(TOUPPER "HAVE_${_HEADER}" _UPPER)
      string(REPLACE "." "_" _HAVE_H ${_UPPER})
      set(${_HAVE_H} 1)
    endforeach()
    set(HAVE_SIGNAL_H 1)
    foreach(_FN
            malloc calloc realloc free bsearch qsort abs memset memcpy memmove memcmp
            wcslen _wcsdup wcsdup wcsstr wcscmp wcsncmp _wcsicmp _wcsnicmp
            strlen _strrev _strupr _strlwr strchr strrchr strstr itoa _ltoa
            _ultoa strtol strtoul strtoll strtod atoi atof strcmp strncmp
            _stricmp _strnicmp sscanf
            acos acosf asin asinf atan atanf atan2 atan2f ceil ceilf
            copysign copysignf cos cosf exp expf fabs fabsf floor floorf fmod fmodf
            log logf log10 log10f lround lroundf pow powf round roundf scalbn scalbnf
            sin sinf sqrt sqrtf tan tanf trunc truncf)
      string(TOUPPER ${_FN} _UPPER)
      set(HAVE_${_UPPER} 1)
    endforeach()
    set(HAVE_ALLOCA 1)
    check_symbol_exists(M_PI math.h HAVE_M_PI)
    target_compile_definitions(sdl-build-options INTERFACE "-D_USE_MATH_DEFINES") # needed for M_PI
    set(STDC_HEADERS 1)
  else()
    set(HAVE_LIBC TRUE)
    set(headers_to_check
        ctype.h
        float.h
        iconv.h
        inttypes.h
        limits.h
        malloc.h
        math.h
        memory.h
        signal.h
        stdarg.h
        stddef.h
        stdint.h
        stdio.h
        stdlib.h
        string.h
        strings.h
        sys/types.h
        wchar.h
    )
    foreach(_HEADER ${headers_to_check})
      string(TOUPPER "HAVE_${_HEADER}" _UPPER)
      string(REGEX REPLACE "[./]" "_" _HAVE_H ${_UPPER})
      check_include_file("${_HEADER}" ${_HAVE_H})
    endforeach()

    set(STDC_HEADER_NAMES "stddef.h;stdarg.h;stdlib.h;string.h;stdio.h;wchar.h;float.h")
    check_include_files("${STDC_HEADER_NAMES}" STDC_HEADERS)
    check_symbol_exists(M_PI math.h HAVE_M_PI)
    # Urho3D - for consistency sake use check_include_file() to check for HAVE_MPROTECT
    check_include_file (sys/mman.h HAVE_MPROTECT)
    foreach(_FN
            strtod malloc calloc realloc free getenv setenv putenv unsetenv
            bsearch qsort abs bcopy memset memcpy memmove memcmp strlen strlcpy strlcat
            _strrev _strupr _strlwr index rindex strchr strrchr strstr strtok_r
            itoa _ltoa _uitoa _ultoa strtol strtoul _i64toa _ui64toa strtoll strtoull
            atoi atof strcmp strncmp _stricmp strcasecmp _strnicmp strncasecmp strcasestr
            wcscmp _wcsdup wcsdup wcslcat wcslcpy wcslen wcsncmp wcsstr
            wcscasecmp _wcsicmp wcsncasecmp _wcsnicmp
            sscanf vsscanf vsnprintf fopen64 fseeko fseeko64 _Exit
            )
      string(TOUPPER ${_FN} _UPPER)
      set(_HAVEVAR "HAVE_${_UPPER}")
      check_symbol_exists("${_FN}" "${STDC_HEADER_NAMES}" ${_HAVEVAR})
    endforeach()

    check_symbol_exists(sigaction "signal.h" HAVE_SIGACTION)
    check_symbol_exists(setjmp "setjmp.h" HAVE_SETJMP)
    check_symbol_exists(nanosleep "time.h" HAVE_NANOSLEEP)
    check_symbol_exists(sysconf "unistd.h" HAVE_SYSCONF)
    check_symbol_exists(sysctlbyname "sys/types.h;sys/sysctl.h" HAVE_SYSCTLBYNAME)
    check_symbol_exists(getauxval "sys/auxv.h" HAVE_GETAUXVAL)
    check_symbol_exists(elf_aux_info "sys/auxv.h" HAVE_ELF_AUX_INFO)
    check_symbol_exists(poll "poll.h" HAVE_POLL)
    check_symbol_exists(memfd_create "sys/mman.h" HAVE_MEMFD_CREATE)
    check_symbol_exists(posix_fallocate "fcntl.h" HAVE_POSIX_FALLOCATE)

    check_library_exists(m pow "" HAVE_LIBM)
    if(HAVE_LIBM)
      set(CMAKE_REQUIRED_LIBRARIES m)
    endif()
    foreach(_FN
              atan atan2 atanf atan2f ceil ceilf copysign copysignf cos cosf
              exp expf fabs fabsf floor floorf fmod fmodf log logf log10 log10f
              lround lroundf pow powf round roundf scalbn scalbnf sin sinf sqrt
              sqrtf tan tanf acos acosf asin asinf trunc truncf)
        string(TOUPPER ${_FN} _UPPER)
        set(_HAVEVAR "HAVE_${_UPPER}")
        check_symbol_exists("${_FN}" "math.h" ${_HAVEVAR})
    endforeach()
    if(HAVE_LIBM)
      set(CMAKE_REQUIRED_LIBRARIES)
      if(NOT VITA)
        list(APPEND EXTRA_LIBS m)
      endif()
    endif()

    if(SDL_SYSTEM_ICONV)
      check_c_source_compiles("
        #define LIBICONV_PLUG 1 /* in case libiconv header is in include path */
        #include <stddef.h>
        #include <iconv.h>
        int main(int argc, char **argv) {
            return !iconv_open(NULL,NULL);
        }" ICONV_IN_LIBC)

      cmake_push_check_state()
      list(APPEND CMAKE_REQUIRED_LIBRARIES iconv)
      check_c_source_compiles("
        #include <stddef.h>
        #include <iconv.h>
        int main(int argc, char **argv) {
            return !iconv_open(NULL,NULL);
        }" ICONV_IN_LIBICONV)
      cmake_pop_check_state()

      if(ICONV_IN_LIBC OR ICONV_IN_LIBICONV)
        set(HAVE_ICONV 1)
        set(HAVE_SYSTEM_ICONV TRUE)
        if(ICONV_IN_LIBICONV AND (SDL_LIBICONV OR (NOT ICONV_IN_LIBC)))
          set(SDL_USE_LIBICONV 1)
          set(HAVE_LIBICONV TRUE)
          list(APPEND EXTRA_LIBS iconv)
        endif()
      endif()
    endif()

    # Urho3D - bug fix - cannot use check_function_exists() with alloca; with this bug fixed, the same check should work on Apple too so no more hardcoding
      check_include_file(alloca.h HAVE_ALLOCA_H)
    if (HAVE_ALLOCA_H)
      check_c_source_compiles ("
        #include <alloca.h>
        int main(int argc, char** argv) { alloca(0); }" HAVE_ALLOCA)
    endif()

    check_struct_has_member("struct sigaction" "sa_sigaction" "signal.h" HAVE_SA_SIGACTION)
  endif()
else()
  if(WINDOWS)
    set(HAVE_STDARG_H 1)
    set(HAVE_STDDEF_H 1)
    check_include_file(stdint.h HAVE_STDINT_H)

    if(MSVC AND USE_CLANG)
      check_c_compiler_flag("/Q_no-use-libirc" HAS_Q_NO_USE_LIBIRC )
    endif()
  endif()
endif()


# Enable/disable various subsystems of the SDL library
foreach(_SUB ${SDL_SUBSYSTEMS})
  string(TOUPPER ${_SUB} _OPT)
  if(NOT SDL_${_OPT})
    set(SDL_${_OPT}_DISABLED 1)
  endif()
endforeach()
if(SDL_HAPTIC)
  if(NOT SDL_JOYSTICK)
    # Haptic requires some private functions from the joystick subsystem.
    message_error("SDL_HAPTIC requires SDL_JOYSTICK, which is not enabled")
  endif()
endif()


# General SDL subsystem options, valid for all platforms
if(SDL_AUDIO)
  # CheckDummyAudio/CheckDiskAudio - valid for all platforms
  if(SDL_DUMMYAUDIO)
    set(SDL_AUDIO_DRIVER_DUMMY 1)
    file(GLOB DUMMYAUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/dummy/*.c)
    list(APPEND SOURCE_FILES ${DUMMYAUDIO_SOURCES})
    set(HAVE_DUMMYAUDIO TRUE)
    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_DISKAUDIO)
    set(SDL_AUDIO_DRIVER_DISK 1)
    file(GLOB DISKAUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/disk/*.c)
    list(APPEND SOURCE_FILES ${DISKAUDIO_SOURCES})
    set(HAVE_DISKAUDIO TRUE)
    set(HAVE_SDL_AUDIO TRUE)
  endif()
endif()

if(UNIX OR APPLE)
  # Relevant for Unix/Darwin only
  set(DYNAPI_NEEDS_DLOPEN 1)
  CheckDLOPEN()
  if(SDL_LOADSO AND HAVE_DLOPEN)
    set(SDL_LOADSO_DLOPEN 1)
    file(GLOB DLOPEN_SOURCES ${SDL2_SOURCE_DIR}/src/loadso/dlopen/*.c)
    list(APPEND SOURCE_FILES ${DLOPEN_SOURCES})
    set(HAVE_SDL_LOADSO TRUE)
  endif()
endif()

if(UNIX OR APPLE OR HAIKU OR RISCOS)
  CheckO_CLOEXEC()
endif()

if(SDL_JOYSTICK)
  if(SDL_VIRTUAL_JOYSTICK)
    set(HAVE_VIRTUAL_JOYSTICK TRUE)
    set(SDL_JOYSTICK_VIRTUAL 1)
    file(GLOB JOYSTICK_VIRTUAL_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/virtual/*.c)
    list(APPEND SOURCE_FILES ${JOYSTICK_VIRTUAL_SOURCES})
  endif()
endif()

if(SDL_VIDEO)
  if(SDL_DUMMYVIDEO)
    set(SDL_VIDEO_DRIVER_DUMMY 1)
    file(GLOB VIDEO_DUMMY_SOURCES ${SDL2_SOURCE_DIR}/src/video/dummy/*.c)
    list(APPEND SOURCE_FILES ${VIDEO_DUMMY_SOURCES})
    set(HAVE_DUMMYVIDEO TRUE)
    set(HAVE_SDL_VIDEO TRUE)
  endif()
  if(SDL_OFFSCREEN)
    set(SDL_VIDEO_DRIVER_OFFSCREEN 1)
    file(GLOB VIDEO_OFFSCREEN_SOURCES ${SDL2_SOURCE_DIR}/src/video/offscreen/*.c)
    list(APPEND SOURCE_FILES ${VIDEO_OFFSCREEN_SOURCES})
    set(HAVE_OFFSCREEN TRUE)
    set(HAVE_SDL_VIDEO TRUE)
  endif()
endif()

# Platform-specific options and settings
if(ANDROID)
  file(GLOB ANDROID_CORE_SOURCES ${SDL2_SOURCE_DIR}/src/core/android/*.c)
  list(APPEND SOURCE_FILES ${ANDROID_CORE_SOURCES} ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
  set_property(SOURCE "${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-declaration-after-statement")

  if(SDL_MISC)
    file(GLOB ANDROID_MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  # SDL_spinlock.c Needs to be compiled in ARM mode.
  # There seems to be no better way currently to set the ARM mode.
  # see: https://issuetracker.google.com/issues/62264618
  # Another option would be to set ARM mode to all compiled files
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_FLAGS "-Werror=unused-command-line-argument")
  check_c_compiler_flag(-marm HAVE_ARM_MODE)
  if(HAVE_ARM_MODE)
    set_property(SOURCE "${SDL2_SOURCE_DIR}/src/atomic/SDL_spinlock.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -marm")
  endif()
  cmake_pop_check_state()

  file(GLOB ANDROID_MAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/android/*.c)
  list(APPEND SDLMAIN_SOURCES ${ANDROID_MAIN_SOURCES})

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_ANDROID 1)
    file(GLOB ANDROID_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_AUDIO_SOURCES})

    set(SDL_AUDIO_DRIVER_OPENSLES 1)
    file(GLOB OPENSLES_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/openslES/*.c)
    list(APPEND SOURCE_FILES ${OPENSLES_AUDIO_SOURCES})

    list(APPEND EXTRA_LIBS ${ANDROID_DL_LIBRARY} OpenSLES)

    set(SDL_AUDIO_DRIVER_AAUDIO 1)
    file(GLOB AAUDIO_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/aaudio/*.c)
    list(APPEND SOURCE_FILES ${AAUDIO_AUDIO_SOURCES})

    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_ANDROID 1)
    file(GLOB ANDROID_FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()
  if(SDL_HAPTIC)
    set(SDL_HAPTIC_ANDROID 1)
    file(GLOB ANDROID_HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_HAPTIC_SOURCES})
    set(HAVE_SDL_HAPTIC TRUE)
  endif()
  if(SDL_HIDAPI)
    CheckHIDAPI()
  endif()
  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_ANDROID 1)
    file(GLOB ANDROID_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/android/*.c ${SDL2_SOURCE_DIR}/src/joystick/steam/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()
  if(SDL_LOADSO)
    set(SDL_LOADSO_DLOPEN 1)
    file(GLOB LOADSO_SOURCES ${SDL2_SOURCE_DIR}/src/loadso/dlopen/*.c)
    list(APPEND SOURCE_FILES ${LOADSO_SOURCES})
    set(HAVE_SDL_LOADSO TRUE)
  endif()
  if(SDL_POWER)
    set(SDL_POWER_ANDROID 1)
    file(GLOB ANDROID_POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()
  if(SDL_LOCALE)
    file(GLOB ANDROID_LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()
  if(SDL_TIMERS)
    set(SDL_TIMER_UNIX 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/unix/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()
  if(SDL_SENSOR)
    set(SDL_SENSOR_ANDROID 1)
    set(HAVE_SDL_SENSORS TRUE)
    file(GLOB ANDROID_SENSOR_SOURCES ${SDL2_SOURCE_DIR}/src/sensor/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_SENSOR_SOURCES})
  endif()
  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_ANDROID 1)
    file(GLOB ANDROID_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/android/*.c)
    list(APPEND SOURCE_FILES ${ANDROID_VIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)

    # Core stuff
    # find_library(ANDROID_DL_LIBRARY dl)
    # FIXME failing dlopen https://github.com/android-ndk/ndk/issues/929
    list(APPEND EXTRA_LIBS dl log android)
    target_compile_definitions(sdl-build-options INTERFACE "-DGL_GLEXT_PROTOTYPES")

    #enable gles
    if(SDL_OPENGLES)
      set(SDL_VIDEO_OPENGL_EGL 1)
      set(HAVE_OPENGLES TRUE)
      set(SDL_VIDEO_OPENGL_ES 1)
      set(SDL_VIDEO_RENDER_OGL_ES 1)
      set(SDL_VIDEO_OPENGL_ES2 1)
      set(SDL_VIDEO_RENDER_OGL_ES2 1)

      list(APPEND EXTRA_LIBS GLESv1_CM GLESv2)
    endif()

    if(SDL_VULKAN)
      CHECK_C_SOURCE_COMPILES("
      #if defined(__ARM_ARCH) && __ARM_ARCH < 7
      #error Vulkan doesn't work on this configuration
      #endif
      int main(int argc, char **argv) { return 0; }
      " VULKAN_PASSED_ANDROID_CHECKS)
      if(VULKAN_PASSED_ANDROID_CHECKS)
        set(SDL_VIDEO_VULKAN 1)
        set(HAVE_VULKAN TRUE)
      endif()
    endif()
  endif()

  CheckPTHREAD()
  if(SDL_CLOCK_GETTIME)
    set(HAVE_CLOCK_GETTIME 1)
  endif()

elseif(EMSCRIPTEN)
  # Hide noisy warnings that intend to aid mostly during initial stages of porting a new
  # project. Uncomment at will for verbose cross-compiling -I/../ path info.
  target_compile_options(sdl-build-options INTERFACE "-Wno-warn-absolute-paths")

  if(SDL_MISC)
    file(GLOB EMSRIPTEN_MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/emscripten/*.c)
    list(APPEND SOURCE_FILES ${EMSRIPTEN_MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()
  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_EMSCRIPTEN 1)
    file(GLOB EM_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/emscripten/*.c)
    list(APPEND SOURCE_FILES ${EM_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_EMSCRIPTEN 1)
    file(GLOB EM_FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/emscripten/*.c)
    list(APPEND SOURCE_FILES ${EM_FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()
  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_EMSCRIPTEN 1)
    file(GLOB EM_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/emscripten/*.c)
    list(APPEND SOURCE_FILES ${EM_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()
  if(SDL_POWER)
    set(SDL_POWER_EMSCRIPTEN 1)
    file(GLOB EM_POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/emscripten/*.c)
    list(APPEND SOURCE_FILES ${EM_POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()
  if(SDL_LOCALE)
    file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/emscripten/*.c)
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()
  if(SDL_TIMERS)
    set(SDL_TIMER_UNIX 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/unix/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)

    if(SDL_CLOCK_GETTIME)
      set(HAVE_CLOCK_GETTIME 1)
    endif()
  endif()
  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_EMSCRIPTEN 1)
    file(GLOB EM_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/emscripten/*.c)
    list(APPEND SOURCE_FILES ${EM_VIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)

    #enable gles
    if(SDL_OPENGLES)
      set(SDL_VIDEO_OPENGL_EGL 1)
      set(HAVE_OPENGLES TRUE)
      set(SDL_VIDEO_OPENGL_ES2 1)
      set(SDL_VIDEO_RENDER_OGL_ES2 1)
    endif()
  endif()

  CheckPTHREAD()
  CheckLibUnwind()

elseif(UNIX AND NOT APPLE AND NOT RISCOS AND NOT HAIKU)
  if(SDL_AUDIO)
    if(SYSV5 OR SOLARIS OR HPUX)
        set(SDL_AUDIO_DRIVER_SUNAUDIO 1)
        file(GLOB SUN_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/sun/*.c)
        list(APPEND SOURCE_FILES ${SUN_AUDIO_SOURCES})
        set(HAVE_SDL_AUDIO TRUE)
    elseif(NETBSD)
        set(SDL_AUDIO_DRIVER_NETBSD 1)
        file(GLOB NETBSD_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/netbsd/*.c)
        list(APPEND SOURCE_FILES ${NETBSD_AUDIO_SOURCES})
        set(HAVE_SDL_AUDIO TRUE)
    elseif(AIX)
        set(SDL_AUDIO_DRIVER_PAUDIO 1)
        file(GLOB AIX_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/paudio/*.c)
        list(APPEND SOURCE_FILES ${AIX_AUDIO_SOURCES})
        set(HAVE_SDL_AUDIO TRUE)
    elseif(QNX)
        set(SDL_AUDIO_DRIVER_QSA 1)
        file(GLOB QSA_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/qsa/*.c)
        list(APPEND SOURCE_FILES ${QSA_AUDIO_SOURCES})
        list(APPEND EXTRA_LIBS asound)
        set(HAVE_SDL_AUDIO TRUE)
    endif()
    CheckOSS()
    CheckALSA()
    CheckJACK()
    CheckPipewire()
    CheckPulseAudio()
    CheckESD()
    CheckARTS()
    CheckNAS()
    CheckSNDIO()
    CheckFusionSound()
  endif()

  if(SDL_VIDEO)
    # Need to check for Raspberry PI first and add platform specific compiler flags, otherwise the test for GLES fails!
    # Urho3D - issue with CheckRPI on crosscompile, fallback to the old check 
    CheckOldRPI()
    CheckX11()
    CheckDirectFB()
    # Need to check for EGL first because KMSDRM and Wayland depends on it.
    CheckEGL()
    CheckKMSDRM()
    CheckOpenGL()
    CheckOpenGLES()
    CheckWayland()
    CheckVivante()
    # FIXME: implement CheckVulkan()
    if(SDL_VULKAN)
      set(SDL_VIDEO_VULKAN 1)
      set(HAVE_VULKAN TRUE)
    endif()
    CheckQNXScreen()
  endif()

  if(UNIX)
    file(GLOB CORE_UNIX_SOURCES ${SDL2_SOURCE_DIR}/src/core/unix/*.c)
    list(APPEND SOURCE_FILES ${CORE_UNIX_SOURCES})

    check_c_source_compiles("
        #include <linux/input.h>
        #ifndef EVIOCGNAME
        #error EVIOCGNAME() ioctl not available
        #endif
        int main(int argc, char** argv) { return 0; }" HAVE_LINUX_INPUT_H)

    if(LINUX)
      check_c_source_compiles("
          #include <linux/kd.h>
          #include <linux/keyboard.h>
          #include <sys/ioctl.h>
          int main(int argc, char **argv) {
              struct kbentry kbe;
              kbe.kb_table = KG_CTRL;
              ioctl(0, KDGKBENT, &kbe);
              return 0;
          }" HAVE_INPUT_KD)
    elseif(FREEBSD)
      check_c_source_compiles("
          #include <sys/kbio.h>
          #include <sys/ioctl.h>
          int main(int argc, char **argv) {
              accentmap_t accTable;
              ioctl(0, KDENABIO, 1);
              return 0;
          }" HAVE_INPUT_KBIO)
    elseif(OPENBSD OR NETBSD)
      check_c_source_compiles("
          #include <sys/time.h>
          #include <dev/wscons/wsconsio.h>
          #include <dev/wscons/wsksymdef.h>
          #include <dev/wscons/wsksymvar.h>
          #include <sys/ioctl.h>
          int main(int argc, char **argv) {
              struct wskbd_map_data data;
              ioctl(0, WSKBDIO_GETMAP, &data);
              return 0;
          }" HAVE_INPUT_WSCONS)
    endif()

    if(HAVE_LINUX_INPUT_H)
      set(SDL_INPUT_LINUXEV 1)
    endif()

    if(SDL_HAPTIC AND HAVE_LINUX_INPUT_H)
      set(SDL_HAPTIC_LINUX 1)
      file(GLOB HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/linux/*.c)
      list(APPEND SOURCE_FILES ${HAPTIC_SOURCES})
      set(HAVE_SDL_HAPTIC TRUE)
    endif()

    if(HAVE_INPUT_KD)
      set(SDL_INPUT_LINUXKD 1)
    endif()

    if(HAVE_INPUT_KBIO)
      set(SDL_INPUT_FBSDKBIO 1)
    endif()

    if(HAVE_INPUT_WSCONS)
      set(SDL_INPUT_WSCONS 1)
    endif()

    CheckLibUDev()
    check_include_file("sys/inotify.h" HAVE_SYS_INOTIFY_H)
    check_symbol_exists(inotify_init "sys/inotify.h" HAVE_INOTIFY_INIT)
    check_symbol_exists(inotify_init1 "sys/inotify.h" HAVE_INOTIFY_INIT1)

    if(HAVE_SYS_INOTIFY_H AND HAVE_INOTIFY_INIT)
      set(HAVE_INOTIFY 1)
    endif()

    if(PKG_CONFIG_FOUND)
      if(SDL_DBUS)
        pkg_search_module(DBUS dbus-1 dbus)
        if(DBUS_FOUND)
          set(HAVE_DBUS_DBUS_H TRUE)
          target_include_directories(sdl-build-options INTERFACE "${DBUS_INCLUDE_DIRS}")
          # Fcitx need only dbus.
          set(HAVE_FCITX TRUE)
          set(HAVE_DBUS TRUE)
        endif()
      endif()

      if(SDL_IBUS)
        pkg_search_module(IBUS ibus-1.0 ibus)
        find_path(HAVE_SYS_INOTIFY_H NAMES sys/inotify.h)
        if(IBUS_FOUND AND HAVE_SYS_INOTIFY_H)
          set(HAVE_IBUS_IBUS_H TRUE)
          target_include_directories(sdl-build-options INTERFACE "${IBUS_INCLUDE_DIRS}")
          set(HAVE_IBUS TRUE)
        endif()
      endif()

      if (HAVE_IBUS_IBUS_H OR HAVE_FCITX)
        set(SDL_USE_IME 1)
      endif()

      if(FREEBSD AND NOT HAVE_INOTIFY)
        pkg_search_module(INOTIFY libinotify)
        if(INOTIFY_FOUND)
          set(HAVE_INOTIFY 1)
          target_include_directories(sdl-build-options INTERFACE "${INOTIFY_INCLUDE_DIRS}")
          list(APPEND EXTRA_LIBS ${INOTIFY_LIBRARIES})
        endif()
      endif()

    endif()
    CheckLibUnwind()

    if(HAVE_DBUS_DBUS_H)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_dbus.c")
    endif()

    if(SDL_USE_IME)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_ime.c")
    endif()

    if(HAVE_IBUS_IBUS_H)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_ibus.c")
    endif()

    if(HAVE_FCITX)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_fcitx.c")
    endif()

    if(HAVE_LIBUDEV_H)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_udev.c")
    endif()

    if(HAVE_LINUX_INPUT_H)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_evdev.c")
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_evdev_kbd.c")
    endif()

    if(HAVE_INPUT_KBIO)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/freebsd/SDL_evdev_kbd_freebsd.c")
    endif()

    if(HAVE_INPUT_WSCONS)
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/openbsd/SDL_wscons_kbd.c")
      list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/openbsd/SDL_wscons_mouse.c")
    endif()

    # Always compiled for Linux, unconditionally:
    list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_evdev_capabilities.c")
    list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_threadprio.c")
    list(APPEND SOURCE_FILES "${SDL2_SOURCE_DIR}/src/core/linux/SDL_sandbox.c")

    # src/core/unix/*.c is included in a generic if(UNIX) section, elsewhere.
  endif()

  if(SDL_HIDAPI)
    CheckHIDAPI()
  endif()

  if(SDL_JOYSTICK)
    if(FREEBSD OR NETBSD OR OPENBSD OR BSDI)
      CheckUSBHID()
    endif()
    if((LINUX OR FREEBSD) AND HAVE_LINUX_INPUT_H AND NOT ANDROID)
      set(SDL_JOYSTICK_LINUX 1)
      file(GLOB JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/linux/*.c ${SDL2_SOURCE_DIR}/src/joystick/steam/*.c)
      list(APPEND SOURCE_FILES ${JOYSTICK_SOURCES})
      set(HAVE_SDL_JOYSTICK TRUE)
    endif()
  endif()

  CheckPTHREAD()

  if(SDL_CLOCK_GETTIME)
    check_library_exists(c clock_gettime "" FOUND_CLOCK_GETTIME_LIBC)
    if(FOUND_CLOCK_GETTIME_LIBC)
      set(HAVE_CLOCK_GETTIME 1)
    else()
      check_library_exists(rt clock_gettime "" FOUND_CLOCK_GETTIME_LIBRT)
      if(FOUND_CLOCK_GETTIME_LIBRT)
        set(HAVE_CLOCK_GETTIME 1)
        list(APPEND EXTRA_LIBS rt)
      endif()
    endif()
  endif()

  # Urho3D - commented out setting of HAVE_LINUX_VERSION_H compiler define via cflags as that define is not being used at all, besides we prefer add_definitions()

  if(SDL_MISC)
    file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/unix/*.c)
    list(APPEND SOURCE_FILES ${MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  if(SDL_POWER)
    if(LINUX)
      set(SDL_POWER_LINUX 1)
      file(GLOB POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/linux/*.c)
      list(APPEND SOURCE_FILES ${POWER_SOURCES})
      set(HAVE_SDL_POWER TRUE)
    endif()
  endif()

  if(SDL_LOCALE)
    file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/unix/*.c)
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_UNIX 1)
    file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/unix/*.c)
    list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_UNIX 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/unix/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()

  set(SDL_RLD_FLAGS "")
  if(SDL_RPATH AND SDL_SHARED)
    if(BSDI OR FREEBSD OR LINUX OR NETBSD)
      set(CMAKE_REQUIRED_FLAGS "-Wl,--enable-new-dtags")
      check_c_compiler_flag("" HAVE_ENABLE_NEW_DTAGS)
      set(CMAKE_REQUIRED_FLAGS ${ORIG_CMAKE_REQUIRED_FLAGS})
      if(HAVE_ENABLE_NEW_DTAGS)
        set(SDL_RLD_FLAGS "-Wl,-rpath,\${libdir} -Wl,--enable-new-dtags")
      else()
        set(SDL_RLD_FLAGS "-Wl,-rpath,\${libdir}")
      endif()
    elseif(SOLARIS)
      set(SDL_RLD_FLAGS "-R\${libdir}")
    endif()
    # Urho3D - commented out RPATH setup as we have configured ours globally
    set(HAVE_RPATH TRUE)
  endif()

  if(QNX)
    # QNX's *printf() family generates a SIGSEGV if NULL is passed for a string
    # specifier (on purpose), but SDL expects "(null)". Use the built-in
    # implementation.
    set(HAVE_VSNPRINTF 0)
    set(USE_POSIX_SPAWN 1)
  endif()
elseif(WINDOWS)
  find_program(WINDRES windres)

  # Urho3D - replace the windows.h check by using check_include_file() instead of check_c_source_files()
  check_include_file (windows.h HAVE_WIN32_CC)

  file(GLOB CORE_SOURCES ${SDL2_SOURCE_DIR}/src/core/windows/*.c)
  list(APPEND SOURCE_FILES ${CORE_SOURCES})

  if(WINDOWS_STORE)
    file(GLOB WINRT_SOURCE_FILES ${SDL2_SOURCE_DIR}/src/core/winrt/*.c ${SDL2_SOURCE_DIR}/src/core/winrt/*.cpp)
    list(APPEND SOURCE_FILES ${WINRT_SOURCE_FILES})
  endif()

  if(SDL_MISC)
    if(WINDOWS_STORE)
      file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/winrt/*.cpp)
    else()
      file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/windows/*.c)
    endif()
    list(APPEND SOURCE_FILES ${MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  # Check for DirectX
  if(SDL_DIRECTX)
    # Urho3D - bug fix - use our own FindDirectX.cmake module which does not rely on DXSDK_DIR environment variable directly when DirectX SDK is being used
    # Urho3D - TODO - move the find_package(DirectX) call from Urho3D common module to here later after we have refactored the library dependency handling
  endif()

  if(SDL_XINPUT)
    # xinput.h may need windows.h, but does not include it itself.
    check_c_source_compiles("
      #include <windows.h>
      #include <xinput.h>
      int main(int argc, char **argv) { return 0; }" HAVE_XINPUT_H)
    check_c_source_compiles("
      #define COBJMACROS
      #include <windows.gaming.input.h>
      __x_ABI_CWindows_CGaming_CInput_CIGamepadStatics2 *s2;
      int main(int argc, char **argv) { return 0; }" HAVE_WINDOWS_GAMING_INPUT_H)
  endif()

  # headers needed elsewhere
  check_include_file(tpcshrd.h HAVE_TPCSHRD_H)
  check_include_file(roapi.h HAVE_ROAPI_H)
  check_include_file(mmdeviceapi.h HAVE_MMDEVICEAPI_H)
  check_include_file(audioclient.h HAVE_AUDIOCLIENT_H)
  check_include_file(sensorsapi.h HAVE_SENSORSAPI_H)
  check_include_file(shellscalingapi.h HAVE_SHELLSCALINGAPI_H)

  if(SDL_AUDIO)
    if(NOT WINDOWS_STORE)
      set(SDL_AUDIO_DRIVER_WINMM 1)
      file(GLOB WINMM_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/winmm/*.c)
      list(APPEND SOURCE_FILES ${WINMM_AUDIO_SOURCES})
      set(HAVE_SDL_AUDIO TRUE)
    endif()

    if(HAVE_DSOUND_H AND NOT WINDOWS_STORE)
      set(SDL_AUDIO_DRIVER_DSOUND 1)
      file(GLOB DSOUND_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/directsound/*.c)
      list(APPEND SOURCE_FILES ${DSOUND_AUDIO_SOURCES})
      set(HAVE_SDL_AUDIO TRUE)
    endif()

    if(SDL_WASAPI AND HAVE_AUDIOCLIENT_H AND HAVE_MMDEVICEAPI_H)
      set(SDL_AUDIO_DRIVER_WASAPI 1)
      set(HAVE_WASAPI TRUE)
      file(GLOB WASAPI_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/wasapi/*.c)
      if(WINDOWS_STORE)
        list(APPEND WASAPI_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/wasapi/SDL_wasapi_winrt.cpp)
      endif()
      list(APPEND SOURCE_FILES ${WASAPI_AUDIO_SOURCES})
      set(HAVE_SDL_AUDIO TRUE)
    endif()
  endif()

  if(SDL_VIDEO)
    # requires SDL_LOADSO on Windows (IME, DX, etc.)
    if(NOT SDL_LOADSO)
      message_error("SDL_VIDEO requires SDL_LOADSO, which is not enabled")
    endif()
    if(WINDOWS_STORE)
      set(SDL_VIDEO_DRIVER_WINRT 1)
      file(GLOB WIN_VIDEO_SOURCES
        ${SDL2_SOURCE_DIR}/src/video/winrt/*.c
        ${SDL2_SOURCE_DIR}/src/video/winrt/*.cpp
        ${SDL2_SOURCE_DIR}/src/render/direct3d11/*.cpp
        )
    else()
      set(SDL_VIDEO_DRIVER_WINDOWS 1)
      file(GLOB WIN_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/windows/*.c)
    endif()
    list(APPEND SOURCE_FILES ${WIN_VIDEO_SOURCES})

    if(SDL_RENDER_D3D AND HAVE_D3D_H AND NOT WINDOWS_STORE)
      set(SDL_VIDEO_RENDER_D3D 1)
      set(HAVE_RENDER_D3D TRUE)
    endif()
    if(SDL_RENDER_D3D AND HAVE_D3D11_H)
      set(SDL_VIDEO_RENDER_D3D11 1)
      set(HAVE_RENDER_D3D TRUE)
    endif()
    if(SDL_RENDER_D3D AND HAVE_D3D12_H AND NOT WINDOWS_STORE)
      set(SDL_VIDEO_RENDER_D3D12 1)
      set(HAVE_RENDER_D3D TRUE)
    endif()
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  if(SDL_THREADS)
    set(SDL_THREAD_GENERIC_COND_SUFFIX 1)
    set(SDL_THREAD_WINDOWS 1)
    list(APPEND SOURCE_FILES
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_syscond.c
      ${SDL2_SOURCE_DIR}/src/thread/windows/SDL_syscond_cv.c
      ${SDL2_SOURCE_DIR}/src/thread/windows/SDL_sysmutex.c
      ${SDL2_SOURCE_DIR}/src/thread/windows/SDL_syssem.c
      ${SDL2_SOURCE_DIR}/src/thread/windows/SDL_systhread.c
      ${SDL2_SOURCE_DIR}/src/thread/windows/SDL_systls.c)
    set(HAVE_SDL_THREADS TRUE)
  endif()

  if(SDL_SENSOR AND HAVE_SENSORSAPI_H AND NOT WINDOWS_STORE)
    set(SDL_SENSOR_WINDOWS 1)
    set(HAVE_SDL_SENSORS TRUE)
    file(GLOB WINDOWS_SENSOR_SOURCES ${SDL2_SOURCE_DIR}/src/sensor/windows/*.c)
    list(APPEND SOURCE_FILES ${WINDOWS_SENSOR_SOURCES})
  endif()

  if(SDL_POWER)
    if(WINDOWS_STORE)
      set(SDL_POWER_WINRT 1)
      list(APPEND SOURCE_FILES ${SDL2_SOURCE_DIR}/src/power/winrt/SDL_syspower.cpp)
    else()
      set(SDL_POWER_WINDOWS 1)
      list(APPEND SOURCE_FILES ${SDL2_SOURCE_DIR}/src/power/windows/SDL_syspower.c)
      set(HAVE_SDL_POWER TRUE)
    endif()
  endif()

  if(SDL_LOCALE)
    if(WINDOWS_STORE)
      file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/winrt/*.c)
    else()
      file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/windows/*.c)
    endif()
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_WINDOWS 1)
    if(WINDOWS_STORE)
      file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/winrt/*.cpp)
    else()
      file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/windows/*.c)
    endif()
    list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  # Libraries for Win32 native and MinGW
  if(NOT WINDOWS_STORE)
    list(APPEND EXTRA_LIBS kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32)
  endif()

  if(WINDOWS_STORE)
    list(APPEND EXTRA_LIBS
      -nodefaultlib:vccorlib$<$<CONFIG:Debug>:d>
      -nodefaultlib:msvcrt$<$<CONFIG:Debug>:d>
      vccorlib$<$<CONFIG:Debug>:d>.lib
      msvcrt$<$<CONFIG:Debug>:d>.lib
    )
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_WINDOWS 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/windows/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()

  if(SDL_LOADSO)
    set(SDL_LOADSO_WINDOWS 1)
    file(GLOB LOADSO_SOURCES ${SDL2_SOURCE_DIR}/src/loadso/windows/*.c)
    list(APPEND SOURCE_FILES ${LOADSO_SOURCES})
    set(HAVE_SDL_LOADSO TRUE)
  endif()

  file(GLOB CORE_SOURCES ${SDL2_SOURCE_DIR}/src/core/windows/*.c)
  list(APPEND SOURCE_FILES ${CORE_SOURCES})

  if(SDL_VIDEO)
    if(SDL_OPENGL AND NOT WINDOWS_STORE)
      set(SDL_VIDEO_OPENGL 1)
      set(SDL_VIDEO_OPENGL_WGL 1)
      set(SDL_VIDEO_RENDER_OGL 1)
      set(HAVE_OPENGL TRUE)
    endif()

    if(SDL_OPENGLES)
      set(SDL_VIDEO_OPENGL_EGL 1)
      set(SDL_VIDEO_OPENGL_ES2 1)
      set(SDL_VIDEO_RENDER_OGL_ES2 1)
      set(HAVE_OPENGLES TRUE)
    endif()

    if(SDL_VULKAN)
      set(SDL_VIDEO_VULKAN 1)
      set(HAVE_VULKAN TRUE)
    endif()
  endif()

  if(SDL_HIDAPI)
    CheckHIDAPI()
  endif()

  if(SDL_JOYSTICK)
    file(GLOB JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/windows/*.c)
    list(APPEND SOURCE_FILES ${JOYSTICK_SOURCES})

    if(NOT WINDOWS_STORE)
      set(SDL_JOYSTICK_RAWINPUT 1)
    endif()
    if(HAVE_DINPUT_H)
      set(SDL_JOYSTICK_DINPUT 1)
      list(APPEND EXTRA_LIBS dinput8)
    endif()
    if(HAVE_XINPUT_H)
      if(NOT WINDOWS_STORE)
        set(SDL_JOYSTICK_XINPUT 1)
        set(HAVE_XINPUT TRUE)
      endif()
      if(HAVE_WINDOWS_GAMING_INPUT_H)
        set(SDL_JOYSTICK_WGI 1)
      endif()
    endif()
    set(HAVE_SDL_JOYSTICK TRUE)

    if(SDL_HAPTIC)
      if((HAVE_DINPUT_H OR HAVE_XINPUT_H) AND NOT WINDOWS_STORE)
        file(GLOB HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/windows/*.c)
        if(HAVE_DINPUT_H)
          set(SDL_HAPTIC_DINPUT 1)
        endif()
        if(HAVE_XINPUT_H)
          set(SDL_HAPTIC_XINPUT 1)
        endif()
      else()
        file(GLOB HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/dummy/*.c)
        set(SDL_HAPTIC_DUMMY 1)
      endif()
      list(APPEND SOURCE_FILES ${HAPTIC_SOURCES})
      set(HAVE_SDL_HAPTIC TRUE)
    endif()
  endif()

  # Urho3D - commented out adding main entry point for Windows platform
  # Urho3D - commented out '-mwindows' linker flags setup as it appears to be automatically added by CMake; also commented out 'mingw32' library dependency as it does not appear to be needed, at least that the case for MinGW

elseif(APPLE)
  # TODO: rework this all for proper MacOS X, iOS and Darwin support

  # We always need these libs on macOS at the moment.
  # !!! FIXME: we need Carbon for some very old API calls in
  # !!! FIXME:  src/video/cocoa/SDL_cocoakeyboard.c, but we should figure out
  # !!! FIXME:  how to dump those.
  # Urho3D - no cocoa and carbon on ios/tvos
  if(NOT IOS AND NOT TVOS)
    set(SDL_FRAMEWORK_COCOA 1)
    set(SDL_FRAMEWORK_CARBON 1)
  endif()
  set(SDL_FRAMEWORK_CORE_FOUNDATION 1)
  set(SDL_FRAMEWORK_FOUNDATION 1)
  set(SDL_FRAMEWORK_COREVIDEO 1)

  # Requires the darwin file implementation
  if(SDL_FILE)
    file(GLOB EXTRA_SOURCES ${SDL2_SOURCE_DIR}/src/file/cocoa/*.m)
    list(APPEND SOURCE_FILES ${EXTRA_SOURCES})
    set(HAVE_SDL_FILE TRUE)
  endif()

  if(IOS OR TVOS)
    file(GLOB SDLMAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/uikit/*.c)
  endif()

  if(SDL_MISC)
    if(IOS OR TVOS)
      file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/ios/*.m)
    else()
      file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/macosx/*.m)
    endif()
    list(APPEND SOURCE_FILES ${MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_COREAUDIO 1)
    file(GLOB AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/coreaudio/*.m)
    list(APPEND SOURCE_FILES ${AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
    set(SDL_FRAMEWORK_COREAUDIO 1)
    set(SDL_FRAMEWORK_AUDIOTOOLBOX 1)
    set(SDL_FRAMEWORK_AVFOUNDATION 1)
  endif()

  if(SDL_HIDAPI)
    CheckHIDAPI()
  endif()

  if(SDL_JOYSTICK)
    file(GLOB MFI_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/iphoneos/*.m)
    if(IOS OR TVOS)
      file(GLOB JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/steam/*.c)
      set(SDL_JOYSTICK_MFI 1)
      if(IOS)
        set(SDL_FRAMEWORK_COREMOTION 1)
      endif()
      set(SDL_FRAMEWORK_GAMECONTROLLER 1)
      set(SDL_FRAMEWORK_COREHAPTICS 1)
    else()
      file(GLOB JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/darwin/*.c)
      set_property(SOURCE ${MFI_JOYSTICK_SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS " -fobjc-weak")
      check_objc_source_compiles("
          #include <AvailabilityMacros.h>
          #include <TargetConditionals.h>
          #import <Foundation/Foundation.h>
          #import <GameController/GameController.h>
          #if MAC_OS_X_VERSION_MIN_REQUIRED < 1080
          #error GameController framework doesn't work on this configuration
          #endif
          #if TARGET_CPU_X86
          #error GameController framework doesn't work on this configuration
          #endif
          int main() { return 0; }" HAVE_FRAMEWORK_GAMECONTROLLER)
      check_objc_source_compiles("
          #include <AvailabilityMacros.h>
          #include <TargetConditionals.h>
          #import <Foundation/Foundation.h>
          #import <CoreHaptics/CoreHaptics.h>
          int main() { return 0; }" HAVE_FRAMEWORK_COREHAPTICS)
      if(HAVE_FRAMEWORK_GAMECONTROLLER AND HAVE_FRAMEWORK_COREHAPTICS)
        # Only enable MFI if we also have CoreHaptics to ensure rumble works
        set(SDL_JOYSTICK_MFI 1)
        set(SDL_FRAMEWORK_GAMECONTROLLER 1)
        set(SDL_FRAMEWORK_COREHAPTICS 1)
      endif()
      set(SDL_JOYSTICK_IOKIT 1)
      set(SDL_FRAMEWORK_IOKIT 1)
      set(SDL_FRAMEWORK_FF 1)
    endif()
    list(APPEND SOURCE_FILES ${JOYSTICK_SOURCES} ${MFI_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()

  if(SDL_HAPTIC)
    if (IOS OR TVOS)
      file(GLOB HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/dummy/*.c)
      set(SDL_HAPTIC_DUMMY 1)
    else()
      file(GLOB HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/darwin/*.c)
      set(SDL_HAPTIC_IOKIT 1)
      set(SDL_FRAMEWORK_IOKIT 1)
      set(SDL_FRAMEWORK_FF 1)
    endif()
    list(APPEND SOURCE_FILES ${HAPTIC_SOURCES})
    set(HAVE_SDL_HAPTIC TRUE)
  endif()

  if(SDL_POWER)
    if (IOS OR TVOS)
      file(GLOB POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/uikit/*.m)
      set(SDL_POWER_UIKIT 1)
    else()
      file(GLOB POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/macosx/*.c)
      set(SDL_POWER_MACOSX 1)
      set(SDL_FRAMEWORK_IOKIT 1)
    endif()
    list(APPEND SOURCE_FILES ${POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()

  if(SDL_LOCALE)
    file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/macosx/*.m)
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_UNIX 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/unix/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif(SDL_TIMERS)

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_COCOA 1)
    file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/cocoa/*.m)
    list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  if(SDL_SENSOR)
    if(IOS)
      set(SDL_SENSOR_COREMOTION 1)
      set(HAVE_SDL_SENSORS TRUE)
      file(GLOB SENSOR_SOURCES ${SDL2_SOURCE_DIR}/src/sensor/coremotion/*.m)
      list(APPEND SOURCE_FILES ${SENSOR_SOURCES})
    endif()
  endif()

  # iOS hack needed - http://code.google.com/p/ios-cmake/ ?
  if(SDL_VIDEO)
    if (IOS OR TVOS)
      set(SDL_VIDEO_DRIVER_UIKIT 1)
      set(SDL_FRAMEWORK_COREGRAPHICS 1)
      set(SDL_FRAMEWORK_QUARTZCORE 1)
      set(SDL_FRAMEWORK_UIKIT 1)
      set(SDL_IPHONE_KEYBOARD 1)
      set(SDL_IPHONE_LAUNCHSCREEN 1)
      file(GLOB UIKITVIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/uikit/*.m)
      list(APPEND SOURCE_FILES ${UIKITVIDEO_SOURCES})
      set(HAVE_SDL_VIDEO TRUE)
    else()
      CheckCOCOA()
      if(SDL_OPENGL)
        set(SDL_VIDEO_OPENGL 1)
        set(SDL_VIDEO_OPENGL_CGL 1)
        set(SDL_VIDEO_RENDER_OGL 1)
        set(HAVE_OPENGL TRUE)
      endif()
    endif()

    if(SDL_OPENGLES)
      if(IOS OR TVOS)
        set(SDL_FRAMEWORK_OPENGLES 1)
        set(SDL_VIDEO_OPENGL_ES 1)
        set(SDL_VIDEO_RENDER_OGL_ES 1)
      else()
        set(SDL_VIDEO_OPENGL_EGL 1)
      endif()
      set(SDL_VIDEO_OPENGL_ES2 1)
      set(SDL_VIDEO_RENDER_OGL_ES2 1)
      set(HAVE_OPENGLES TRUE)
    endif()

    if(SDL_VULKAN OR SDL_METAL OR SDL_RENDER_METAL)
      check_objc_source_compiles("
        #include <AvailabilityMacros.h>
        #import <Metal/Metal.h>
        #import <QuartzCore/CAMetalLayer.h>

        #if (!TARGET_CPU_X86_64 && !TARGET_CPU_ARM64)
        #error Metal doesn't work on this configuration
        #endif
        int main(int argc, char **argv) { return 0; }" HAVE_FRAMEWORK_METAL)
      if(HAVE_FRAMEWORK_METAL)
        set(SDL_FRAMEWORK_METAL 1)
        set(SDL_FRAMEWORK_QUARTZCORE 1)
        if(SDL_VULKAN)
          set(SDL_VIDEO_VULKAN 1)
          set(HAVE_VULKAN TRUE)
        endif()
        if(SDL_METAL)
          set(SDL_VIDEO_METAL 1)
          set(HAVE_METAL TRUE)
        endif()
        if(SDL_RENDER_METAL)
          file(GLOB RENDER_METAL_SOURCES ${SDL2_SOURCE_DIR}/src/render/metal/*.m)
          list(APPEND SOURCE_FILES ${RENDER_METAL_SOURCES})
          set(SDL_VIDEO_RENDER_METAL 1)
          set(HAVE_RENDER_METAL TRUE)
        endif()
      endif()
    endif()
  endif()

  # Actually load the frameworks at the end so we don't duplicate include.
  if(SDL_FRAMEWORK_COREVIDEO)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,CoreVideo")
  endif()
  if(SDL_FRAMEWORK_COCOA)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,Cocoa")
  endif()
  if(SDL_FRAMEWORK_IOKIT)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,IOKit")
  endif()
  if(SDL_FRAMEWORK_FF)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,ForceFeedback")
  endif()
  if(SDL_FRAMEWORK_CARBON)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,Carbon")
  endif()
  if(SDL_FRAMEWORK_COREAUDIO)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,CoreAudio")
  endif()
  if(SDL_FRAMEWORK_AUDIOTOOLBOX)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,AudioToolbox")
  endif()
  if(SDL_FRAMEWORK_AVFOUNDATION)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,AVFoundation")
  endif()
  if(SDL_FRAMEWORK_COREBLUETOOTH)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,CoreBluetooth")
  endif()
  if(SDL_FRAMEWORK_COREGRAPHICS)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,CoreGraphics")
  endif()
  if(SDL_FRAMEWORK_COREMOTION)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,CoreMotion")
  endif()
  if(SDL_FRAMEWORK_CORE_FOUNDATION)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,CoreFoundation")
  endif()
  if(SDL_FRAMEWORK_FOUNDATION)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,Foundation")
  endif()
  if(SDL_FRAMEWORK_GAMECONTROLLER)
    find_library(GAMECONTROLLER GameController)
    if(GAMECONTROLLER)
      list(APPEND EXTRA_LDFLAGS "-Wl,-weak_framework,GameController")
    endif()
  endif()
  if(SDL_FRAMEWORK_METAL)
    if(IOS OR TVOS)
      list(APPEND EXTRA_LDFLAGS "-Wl,-framework,Metal")
    else()
      list(APPEND EXTRA_LDFLAGS "-Wl,-weak_framework,Metal")
    endif()
  endif()
  if(SDL_FRAMEWORK_OPENGLES)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,OpenGLES")
  endif()
  if(HAVE_OPENGL)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,OpenGL")
  endif()
  if(SDL_FRAMEWORK_QUARTZCORE)
    if(IOS OR TVOS)
      list(APPEND EXTRA_LDFLAGS "-Wl,-framework,QuartzCore")
    else()
      list(APPEND EXTRA_LDFLAGS "-Wl,-weak_framework,QuartzCore")
    endif()
  endif()
  if(SDL_FRAMEWORK_UIKIT)
    list(APPEND EXTRA_LDFLAGS "-Wl,-framework,UIKit")
  endif()
  if(SDL_FRAMEWORK_COREHAPTICS)
    find_library(COREHAPTICS CoreHaptics)
    if(COREHAPTICS)
      list(APPEND EXTRA_LDFLAGS "-Wl,-weak_framework,CoreHaptics")
    endif()
  endif()

  CheckPTHREAD()

elseif(HAIKU)
  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_HAIKU 1)
    file(GLOB HAIKU_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/haiku/*.cc)
    list(APPEND SOURCE_FILES ${HAIKU_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
  endif()

  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_HAIKU 1)
    file(GLOB HAIKU_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/haiku/*.cc)
    list(APPEND SOURCE_FILES ${HAIKU_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()

  if(SDL_MISC)
    file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/haiku/*.cc)
    list(APPEND SOURCE_FILES ${MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_HAIKU 1)
    file(GLOB HAIKUVIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/haiku/*.cc)
    list(APPEND SOURCE_FILES ${HAIKUVIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)

    if(SDL_OPENGL)
      # TODO: Use FIND_PACKAGE(OpenGL) instead
      set(SDL_VIDEO_OPENGL 1)
      set(SDL_VIDEO_OPENGL_HAIKU 1)
      set(SDL_VIDEO_RENDER_OGL 1)
      list(APPEND EXTRA_LIBS GL)
      set(HAVE_OPENGL TRUE)
    endif()
  endif()

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_HAIKU 1)
    file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/haiku/*.cc)
    list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_HAIKU 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/haiku/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()

  if(SDL_POWER)
    set(SDL_POWER_HAIKU 1)
    file(GLOB HAIKU_POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/haiku/*.c)
    list(APPEND SOURCE_FILES ${HAIKU_POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()

  if(SDL_LOCALE)
    file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/haiku/*.cc)
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()

  file(GLOB MAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/haiku/*.cc)
  list(APPEND SOURCE_FILES ${MAIN_SOURCES})

  CheckPTHREAD()
  list(APPEND EXTRA_LIBS root be media game device textencoding)

elseif(RISCOS)
  if(SDL_MISC)
    file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/riscos/*.c)
    list(APPEND SOURCE_FILES ${MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_RISCOS 1)
    file(GLOB RISCOSVIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/riscos/*.c)
    list(APPEND SOURCE_FILES ${RISCOSVIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_RISCOS 1)
    file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/riscos/*.c)
    list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_UNIX 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/unix/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)

    if(SDL_CLOCK_GETTIME)
      set(HAVE_CLOCK_GETTIME 1)
    endif()
  endif()

  CheckPTHREAD()

  if(SDL_AUDIO)
    CheckOSS()
  endif()

elseif(VITA)
  # SDL_spinlock.c Needs to be compiled in ARM mode.
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_FLAGS "-Werror=unused-command-line-argument")
  check_c_compiler_flag(-marm HAVE_ARM_MODE)
  if(HAVE_ARM_MODE)
    set_property(SOURCE "${SDL2_SOURCE_DIR}/src/atomic/SDL_spinlock.c" APPEND_STRING PROPERTY COMPILE_FLAGS " -marm")
  endif()
  cmake_pop_check_state()

  if(SDL_MISC)
    file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/vita/*.c)
    list(APPEND SOURCE_FILES ${MISC_SOURCES})
    set(HAVE_SDL_MISC TRUE)
  endif()

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_VITA 1)
    file(GLOB VITA_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/vita/*.c)
    list(APPEND SOURCE_FILES ${VITA_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_VITA 1)
    file(GLOB VITA_FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/vita/*.c)
    list(APPEND SOURCE_FILES ${VITA_FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()
  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_VITA 1)
    file(GLOB VITA_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/vita/*.c)
    list(APPEND SOURCE_FILES ${VITA_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()
  if(SDL_POWER)
    set(SDL_POWER_VITA 1)
    file(GLOB VITA_POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/vita/*.c)
    list(APPEND SOURCE_FILES ${VITA_POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()
  if(SDL_THREADS)
    set(SDL_THREAD_VITA 1)
    list(APPEND SOURCE_FILES
      ${SDL2_SOURCE_DIR}/src/thread/vita/SDL_sysmutex.c
      ${SDL2_SOURCE_DIR}/src/thread/vita/SDL_syssem.c
      ${SDL2_SOURCE_DIR}/src/thread/vita/SDL_systhread.c
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_syscond.c
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_systls.c)
    set(HAVE_SDL_THREADS TRUE)
  endif()
  if(SDL_LOCALE)
    file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/vita/*.c)
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()
  if(SDL_TIMERS)
    set(SDL_TIMER_VITA 1)
    file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/vita/*.c)
    list(APPEND SOURCE_FILES ${TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()
  if(SDL_SENSOR)
    set(SDL_SENSOR_VITA 1)
    set(HAVE_SDL_SENSORS TRUE)
    file(GLOB VITA_SENSOR_SOURCES ${SDL2_SOURCE_DIR}/src/sensor/vita/*.c)
    list(APPEND SOURCE_FILES ${VITA_SENSOR_SOURCES})
  endif()
  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_VITA 1)
    file(GLOB VITA_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/vita/*.c)
    list(APPEND SOURCE_FILES ${VITA_VIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)

    if(VIDEO_VITA_PIB)
      check_include_file(pib.h HAVE_PIGS_IN_BLANKET_H)

      if(HAVE_PIGS_IN_BLANKET_H)
        set(SDL_VIDEO_OPENGL_ES2 1)
        list(APPEND EXTRA_LIBS
          pib
          libScePiglet_stub_weak
          taihen_stub_weak
          SceShaccCg_stub_weak
        )
        set(HAVE_VIDEO_VITA_PIB ON)
        set(SDL_VIDEO_VITA_PIB 1)
      else()
        set(HAVE_VIDEO_VITA_PIB OFF)
      endif()
    endif()

    if(VIDEO_VITA_PVR)
      check_include_file(gpu_es4/psp2_pvr_hint.h HAVE_PVR_H)
      if(HAVE_PVR_H)
        target_compile_definitions(sdl-build-options INTERFACE "-D__psp2__")
        set(SDL_VIDEO_OPENGL_EGL 1)
        set(HAVE_OPENGLES TRUE)
        set(SDL_VIDEO_OPENGL_ES 1)
        set(SDL_VIDEO_RENDER_OGL_ES 1)
        set(SDL_VIDEO_OPENGL_ES2 1)
        set(SDL_VIDEO_RENDER_OGL_ES2 1)

        list(APPEND EXTRA_LIBS
          libgpu_es4_ext_stub_weak
          libIMGEGL_stub_weak
          SceIme_stub
        )

        set(HAVE_VIDEO_VITA_PVR ON)
        set(SDL_VIDEO_VITA_PVR 1)

        if(SDL_OPENGL)
          check_include_file(gl4esinit.h HAVE_GL4ES_H)
          if(HAVE_GL4ES_H)
            set(HAVE_OPENGL TRUE)
            set(SDL_VIDEO_OPENGL 1)
            set(SDL_VIDEO_RENDER_OGL 1)
            list(APPEND EXTRA_LIBS libGL_stub)
            set(SDL_VIDEO_VITA_PVR_OGL 1)
          endif()
        endif()

      else()
        set(HAVE_VIDEO_VITA_PVR OFF)
      endif()
    endif()

    set(SDL_VIDEO_RENDER_VITA_GXM 1)

    list(APPEND EXTRA_LIBS
      SceGxm_stub
      SceDisplay_stub
      SceCtrl_stub
      SceAppMgr_stub
      SceAppUtil_stub
      SceAudio_stub
      SceAudioIn_stub
      SceSysmodule_stub
      SceDisplay_stub
      SceCtrl_stub
      SceIofilemgr_stub
      SceCommonDialog_stub
      SceTouch_stub
      SceHid_stub
      SceMotion_stub
      ScePower_stub
      SceProcessmgr_stub
      m
    )
  endif()

  set(HAVE_ARMSIMD TRUE)
#  set(SDL_ARM_SIMD_BLITTERS 1)
#  file(GLOB ARMSIMD_SOURCES ${SDL2_SOURCE_DIR}/src/video/arm/pixman-arm-simd*.S)
#  list(APPEND SOURCE_FILES ${ARMSIMD_SOURCES})

  set(HAVE_ARMNEON TRUE)
#  set(SDL_ARM_NEON_BLITTERS 1)
#  file(GLOB ARMNEON_SOURCES ${SDL2_SOURCE_DIR}/src/video/arm/pixman-arm-neon*.S)
#  list(APPEND SOURCE_FILES ${ARMNEON_SOURCES})

#  set_property(SOURCE ${SDL2_SOURCE_DIR}/src/video/arm/pixman-arm-simd-asm.S PROPERTY LANGUAGE C)
#  set_property(SOURCE ${SDL2_SOURCE_DIR}/src/video/arm/pixman-arm-neon-asm.S PROPERTY LANGUAGE C)

  target_compile_definitions(sdl-build-options INTERFACE "-D__VITA__")

#  CheckPTHREAD()

elseif(PSP)
 file(GLOB PSP_MAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/psp/*.c)
 list(APPEND SDLMAIN_SOURCES ${PSP_MAIN_SOURCES})

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_PSP 1)
    file(GLOB PSP_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_PSP 1)
    file(GLOB PSP_FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()
  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_PSP 1)
    file(GLOB PSP_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()
  if(SDL_POWER)
    set(SDL_POWER_PSP 1)
    file(GLOB PSP_POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()
  if(SDL_THREADS)
    set(SDL_THREAD_PSP 1)
    file(GLOB PSP_THREAD_SOURCES
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_systls.c
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_syscond.c
      ${SDL2_SOURCE_DIR}/src/thread/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_THREAD_SOURCES})
    set(HAVE_SDL_THREADS TRUE)
  endif()
  if(SDL_TIMERS)
    set(SDL_TIMER_PSP 1)
    file(GLOB PSP_TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()
  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_PSP 1)
    set(SDL_VIDEO_RENDER_PSP 1)
    file(GLOB PSP_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/psp/*.c)
    list(APPEND SOURCE_FILES ${PSP_VIDEO_SOURCES})
    set(SDL_VIDEO_OPENGL 1)
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  list(APPEND EXTRA_LIBS
      GL
      pspvram
      pspaudio
      pspvfpu
      pspdisplay
      pspgu
      pspge
      psphprm
      pspctrl
      psppower
  )
  if(NOT SDL2_DISABLE_SDL2MAIN)
    list(INSERT SDL_LIBS 0 "-lSDL2main")
  endif()

elseif(PS2)
  list(APPEND EXTRA_CFLAGS "-DPS2" "-D__PS2__" "-I$ENV{PS2SDK}/ports/include" "-I$ENV{PS2DEV}/gsKit/include")

  file(GLOB PS2_MAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/ps2/*.c)
  set(SDLMAIN_SOURCES ${SDLMAIN_SOURCES} ${PS2_MAIN_SOURCES})

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_PS2 1)
    file(GLOB PS2_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/ps2/*.c)
    set(SOURCE_FILES ${SOURCE_FILES} ${PS2_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
  endif()
  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_PS2 1)
    file(GLOB PS2_FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/ps2/*.c)
    list(APPEND SOURCE_FILES ${PS2_FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()
  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_PS2 1)
    file(GLOB PS2_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/ps2/*.c)
    list(APPEND SOURCE_FILES ${PS2_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()
  if(SDL_THREADS)
    set(SDL_THREAD_PS2 1)
    file(GLOB PS2_THREAD_SOURCES
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_systls.c
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_sysmutex.c
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_syscond.c
      ${SDL2_SOURCE_DIR}/src/thread/ps2/*.c)
    list(APPEND SOURCE_FILES ${PS2_THREAD_SOURCES})
    set(HAVE_SDL_THREADS TRUE)
  endif()
  if(SDL_TIMERS)
    set(SDL_TIMER_PS2 1)
    file(GLOB PS2_TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/ps2/*.c)
    list(APPEND SOURCE_FILES ${PS2_TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()
  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_PS2 1)
    set(SDL_VIDEO_RENDER_PS2 1)
    file(GLOB PS2_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/ps2/*.c ${SDL2_SOURCE_DIR}/src/render/ps2/*.c)
    list(APPEND SOURCE_FILES ${PS2_VIDEO_SOURCES})
    set(SDL_VIDEO_OPENGL 0)
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  list(APPEND EXTRA_LIBS
    patches
    gskit
    dmakit
    ps2_drivers
  )

elseif(OS2)
  list(APPEND EXTRA_CFLAGS "-DOS2EMX_PLAIN_CHAR")

  file(GLOB CORE_SOURCES ${SDL2_SOURCE_DIR}/src/core/os2/*.c)
  list(APPEND SOURCE_FILES ${CORE_SOURCES})
  if(NOT (HAVE_ICONV AND HAVE_ICONV_H))
    file(GLOB CORE_SOURCES ${SDL2_SOURCE_DIR}/src/core/os2/geniconv/*.c)
    list(APPEND SOURCE_FILES ${CORE_SOURCES})
  endif()

  if(SDL_THREADS)
    set(SDL_THREAD_OS2 1)
    file(GLOB OS2_THREAD_SOURCES ${SDL2_SOURCE_DIR}/src/thread/os2/*.c)
    list(APPEND SOURCE_FILES ${OS2_THREAD_SOURCES})
    set(HAVE_SDL_THREADS TRUE)
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_UNIX 1)
    file(GLOB OS2_TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/os2/*.c)
    list(APPEND SOURCE_FILES ${OS2_TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()

  if(SDL_LOADSO)
    set(SDL_LOADSO_OS2 1)
    file(GLOB OS2_LOADSO_SOURCES ${SDL2_SOURCE_DIR}/src/loadso/os2/*.c)
    list(APPEND SOURCE_FILES ${OS2_LOADSO_SOURCES})
    set(HAVE_SDL_LOADSO TRUE)
  endif()

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_OS2 1)
    file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/os2/*.c)
    list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  if(SDL_LOCALE)
    file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/unix/*.c)
    list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()

  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_OS2 1)
    file(GLOB OS2_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/os2/*.c)
    list(APPEND SOURCE_FILES ${OS2_VIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_OS2 1)
    file(GLOB OS2_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/os2/*.c)
    list(APPEND SOURCE_FILES ${OS2_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
    list(APPEND EXTRA_LIBS mmpm2)
  endif()

  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_OS2 1)
    file(GLOB OS2_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/os2/*.c)
    list(APPEND SOURCE_FILES ${OS2_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()

  if(SDL_HIDAPI)
    CheckHIDAPI()
  endif()

elseif(N3DS)
  file(GLOB N3DS_MAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/n3ds/*.c)
  set(SDLMAIN_SOURCES ${SDLMAIN_SOURCES} ${N3DS_MAIN_SOURCES})

  if(SDL_AUDIO)
    set(SDL_AUDIO_DRIVER_N3DS 1)
    file(GLOB N3DS_AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_AUDIO_SOURCES})
    set(HAVE_SDL_AUDIO TRUE)
  endif()

  if(SDL_FILESYSTEM)
    set(SDL_FILESYSTEM_N3DS 1)
    file(GLOB N3DS_FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_FILESYSTEM_SOURCES})
    set(HAVE_SDL_FILESYSTEM TRUE)
  endif()

  if(SDL_JOYSTICK)
    set(SDL_JOYSTICK_N3DS 1)
    file(GLOB N3DS_JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_JOYSTICK_SOURCES})
    set(HAVE_SDL_JOYSTICK TRUE)
  endif()

  if(SDL_POWER)
    set(SDL_POWER_N3DS 1)
    file(GLOB N3DS_POWER_SOURCES ${SDL2_SOURCE_DIR}/src/power/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_POWER_SOURCES})
    set(HAVE_SDL_POWER TRUE)
  endif()

  if(SDL_THREADS)
    set(SDL_THREAD_N3DS 1)
    file(GLOB N3DS_THREAD_SOURCES ${SDL2_SOURCE_DIR}/src/thread/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_THREAD_SOURCES}
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_systls.c
      ${SDL2_SOURCE_DIR}/src/thread/generic/SDL_syscond.c)
    set(HAVE_SDL_THREADS TRUE)
  endif()

  if(SDL_TIMERS)
    set(SDL_TIMER_N3DS 1)
    file(GLOB N3DS_TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_TIMER_SOURCES})
    set(HAVE_SDL_TIMERS TRUE)
  endif()

  if(SDL_SENSOR)
    set(SDL_SENSOR_N3DS 1)
    file(GLOB N3DS_SENSOR_SOURCES ${SDL2_SOURCE_DIR}/src/sensor/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_SENSOR_SOURCES})
    set(HAVE_SDL_SENSORS TRUE)
  endif()

  if(SDL_VIDEO)
    set(SDL_VIDEO_DRIVER_N3DS 1)
    file(GLOB N3DS_VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_VIDEO_SOURCES})
    set(HAVE_SDL_VIDEO TRUE)
  endif()

  if(SDL_LOCALE)
    file(GLOB N3DS_LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_LOCALE_SOURCES})
    set(HAVE_SDL_LOCALE TRUE)
  endif()

  # Requires the n3ds file implementation
  if(SDL_FILE)
    file(GLOB N3DS_FILE_SOURCES ${SDL2_SOURCE_DIR}/src/file/n3ds/*.c)
    list(APPEND SOURCE_FILES ${N3DS_FILE_SOURCES})
    set(HAVE_SDL_FILE TRUE)
  else()
    message_error("SDL_FILE must be enabled to build on N3DS")
  endif()

  if(NOT SDL2_DISABLE_SDL2MAIN)
    list(INSERT SDL_LIBS 0 "-lSDL2main")
  endif()

  foreach(lib ${CMAKE_C_STANDARD_LIBRARIES})
    if(lib MATCHES "^-l")
      string(SUBSTRING "${lib}" 2 -1 lib)
    endif()
    list(APPEND EXTRA_LIBS ${lib})
  endforeach()
endif()

if(HAVE_VULKAN AND NOT SDL_LOADSO)
  message(STATUS "Vulkan support is available, but disabled because there's no loadso.")
  set(HAVE_VULKAN FALSE)
  set(SDL_VIDEO_VULKAN 0)
endif()

# Platform-independent options
CheckLibSampleRate()

# Dummies
# configure.ac does it differently:
# if not have X
#   if enable_X {  SDL_X_DISABLED = 1 }
#   [add dummy sources]
# so it always adds a dummy, without checking, if it was actually requested.
# This leads to missing internal references on building, since the
# src/X/*.c does not get included.
if(NOT HAVE_SDL_AUDIO)
  set(SDL_AUDIO_DRIVER_DUMMY 1)
  file(GLOB AUDIO_SOURCES ${SDL2_SOURCE_DIR}/src/audio/dummy/*.c)
  list(APPEND SOURCE_FILES ${AUDIO_SOURCES})
endif()
if(NOT HAVE_SDL_VIDEO)
  set(SDL_VIDEO_DRIVER_DUMMY 1)
  file(GLOB VIDEO_SOURCES ${SDL2_SOURCE_DIR}/src/video/dummy/*.c)
  list(APPEND SOURCE_FILES ${VIDEO_SOURCES})
endif()
if(NOT HAVE_SDL_JOYSTICK)
  set(SDL_JOYSTICK_DUMMY 1)
  file(GLOB JOYSTICK_SOURCES ${SDL2_SOURCE_DIR}/src/joystick/dummy/*.c)
  list(APPEND SOURCE_FILES ${JOYSTICK_SOURCES})
endif()
if(NOT HAVE_SDL_HAPTIC)
  set(SDL_HAPTIC_DUMMY 1)
  file(GLOB HAPTIC_SOURCES ${SDL2_SOURCE_DIR}/src/haptic/dummy/*.c)
  list(APPEND SOURCE_FILES ${HAPTIC_SOURCES})
endif()
if(NOT HAVE_SDL_SENSORS)
  set(SDL_SENSOR_DUMMY 1)
  file(GLOB SENSORS_SOURCES ${SDL2_SOURCE_DIR}/src/sensor/dummy/*.c)
  list(APPEND SOURCE_FILES ${SENSORS_SOURCES})
endif()
if(NOT HAVE_SDL_LOADSO)
  set(SDL_LOADSO_DUMMY 1)
  file(GLOB LOADSO_SOURCES ${SDL2_SOURCE_DIR}/src/loadso/dummy/*.c)
  list(APPEND SOURCE_FILES ${LOADSO_SOURCES})
endif()
if(NOT HAVE_SDL_FILESYSTEM)
  set(SDL_FILESYSTEM_DUMMY 1)
  file(GLOB FILESYSTEM_SOURCES ${SDL2_SOURCE_DIR}/src/filesystem/dummy/*.c)
  list(APPEND SOURCE_FILES ${FILESYSTEM_SOURCES})
endif()
if(NOT HAVE_SDL_LOCALE)
  set(SDL_LOCALE_DUMMY 1)
  file(GLOB LOCALE_SOURCES ${SDL2_SOURCE_DIR}/src/locale/dummy/*.c)
  list(APPEND SOURCE_FILES ${LOCALE_SOURCES})
endif()
if(NOT HAVE_SDL_MISC)
  set(SDL_MISC_DUMMY 1)
  file(GLOB MISC_SOURCES ${SDL2_SOURCE_DIR}/src/misc/dummy/*.c)
  list(APPEND SOURCE_FILES ${MISC_SOURCES})
endif()

# We always need to have threads and timers around
if(NOT HAVE_SDL_THREADS)
  # The emscripten platform has been carefully vetted to work without threads
  if (EMSCRIPTEN)
    set(SDL_THREADS_DISABLED 1)
    file(GLOB THREADS_SOURCES ${SDL2_SOURCE_DIR}/src/thread/generic/*.c)
    list(APPEND SOURCE_FILES ${THREADS_SOURCES})
  else()
    message_error("Threads are needed by many SDL subsystems and may not be disabled")
  endif()
endif()
if(NOT HAVE_SDL_TIMERS)
  set(SDL_TIMER_DUMMY 1)
  file(GLOB TIMER_SOURCES ${SDL2_SOURCE_DIR}/src/timer/dummy/*.c)
  list(APPEND SOURCE_FILES ${TIMER_SOURCES})
endif()

if(NOT SDLMAIN_SOURCES)
  file(GLOB SDLMAIN_SOURCES ${SDL2_SOURCE_DIR}/src/main/dummy/*.c)
endif()

if(SDL_WERROR)
  if(MSVC)
    cmake_push_check_state(RESET)
    check_c_compiler_flag(/WX HAVE_WX)
    if(HAVE_WX)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif()
  elseif(USE_GCC OR USE_CLANG OR USE_INTELCC)
    cmake_push_check_state(RESET)
    check_c_compiler_flag(-Werror HAVE_WERROR)
    if(HAVE_WERROR)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
      set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -Werror")
    endif()
    cmake_pop_check_state()
  endif()
endif()

# Append the -MMD -MT flags
# if(DEPENDENCY_TRACKING)
#   if(COMPILER_IS_GNUCC)
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MMD -MT \$@")
#   endif()
# endif()

# Urho3D - use 'generated' path suffix for generated config header file (need the suffix as we support both out-of-source and non out-of-source build tree)
configure_file("${SDL2_SOURCE_DIR}/include/SDL_config.h.cmake" "${SDL2_BINARY_DIR}/include/generated/SDL_config.h")
# Urho3D - use 'generated' path suffix for generated config header file, the suffic prevents overwriting the header file with the same name in the source tree in case of non out-of-source build tree is being used
add_definitions(-DUSING_GENERATED_CONFIG_H)
set (INCLUDE_DIRS ${SDL2_BINARY_DIR}/include/generated ${SDL2_SOURCE_DIR}/include)

# Prepare the flags and remove duplicates
if(EXTRA_LDFLAGS)
  list(REMOVE_DUPLICATES EXTRA_LDFLAGS)
endif()
if(EXTRA_LIBS)
  list(REMOVE_DUPLICATES EXTRA_LIBS)
endif()
if(EXTRA_CFLAGS)
  list(REMOVE_DUPLICATES EXTRA_CFLAGS)
endif()
listtostr(EXTRA_CFLAGS _EXTRA_CFLAGS)
set(EXTRA_CFLAGS ${_EXTRA_CFLAGS})

if(USE_GCC OR USE_CLANG)
  string(REGEX REPLACE "(^| )-I" "\\1 -isystem" EXTRA_CFLAGS "${EXTRA_CFLAGS}")
endif()

# Compat helpers for the configuration files

# Urho3D - don't need to source from git and we use generated config header file

if(CMAKE_STATIC_LIBRARY_PREFIX STREQUAL "" AND CMAKE_STATIC_LIBRARY_SUFFIX STREQUAL ".lib")
  # Avoid conflict between the dll import library and the static library
  set(sdl_static_libname "${LIBNAME}-static")
else()
  set(sdl_static_libname "${LIBNAME}")
endif()

# Urho3D - remove all installation stuff

# Clean up the different lists
listtostr(EXTRA_LIBS _EXTRA_LIBS "-l")
set(SDL_STATIC_LIBS ${SDL_LIBS} ${EXTRA_LDFLAGS} ${_EXTRA_LIBS})
list(REMOVE_DUPLICATES SDL_STATIC_LIBS)
listtostr(SDL_STATIC_LIBS _SDL_STATIC_LIBS)
set(SDL_STATIC_LIBS ${_SDL_STATIC_LIBS})
listtostr(SDL_LIBS _SDL_LIBS)
set(SDL_LIBS ${_SDL_LIBS})
listtostr(SDL_CFLAGS _SDL_CFLAGS "")
set(SDL_CFLAGS ${_SDL_CFLAGS})
string(REGEX REPLACE "-lSDL2( |$)" "-l${sdl_static_libname} " SDL_STATIC_LIBS "${SDL_STATIC_LIBS}")
if(NOT SDL_SHARED)
  string(REGEX REPLACE "-lSDL2( |$)" "-l${sdl_static_libname} " SDL_LIBS "${SDL_LIBS}")
endif()
listtostr(PKGCONFIG_DEPENDS PKGCONFIG_DEPENDS)

# Urho3D - commented out pkg-config configuration file generation
# Urho3D - TODO - utilize the populated variables for pkg-config configuration file generation for Urho3D.pc

macro(check_add_debug_flag FLAG SUFFIX)
    check_c_compiler_flag(${FLAG} HAS_C_FLAG_${SUFFIX})
    if (HAS_C_FLAG_${SUFFIX})
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${FLAG}")
    endif()

    check_cxx_compiler_flag(${FLAG} HAS_CXX_${SUFFIX})
    if (HAS_CXX_${SUFFIX})
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG}")
    endif()
endmacro()

macro(asan_check_add_debug_flag ASAN_FLAG)
    check_add_debug_flag("-fsanitize=${ASAN_FLAG}" "${ASAN_FLAG}")
    if(HAS_C_${ASAN_FLAG} OR HAS_CXX_${ASAN_FLAG})
      set(HAVE_ASAN ON)
    endif()
endmacro()

macro(asan_check_add_debug_flag2 ASAN_FLAG)
    # for some sanitize flags we have to manipulate the CMAKE_REQUIRED_LIBRARIES:
    # http://cmake.3232098.n2.nabble.com/CHECK-CXX-COMPILER-FLAG-doesn-t-give-correct-result-for-fsanitize-address-tp7600216p7600217.html

    set(FLAG "-fsanitize=${ASAN_FLAG}")

    set (STORED_REQLIBS ${CMAKE_REQUIRED_LIBRARIES})
    set (CMAKE_REQUIRED_LIBRARIES "${FLAG};asan")
    check_c_compiler_flag (${FLAG} HAS_C_FLAG_${ASAN_FLAG})
    check_cxx_compiler_flag (${FLAG} HAS_CXX_FLAG_${ASAN_FLAG})
    set (CMAKE_REQUIRED_LIBRARIES ${STORED_REQLIBS})

    if (HAS_C_FLAG_${ASAN_FLAG})
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${FLAG}")
    endif()

    if (HAS_CXX_${ASAN_FLAG})
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG}")
    endif()

    if(HAS_C_${ASAN_FLAG} OR HAS_CXX_${ASAN_FLAG})
      set(HAVE_ASAN ON)
    endif()
endmacro()

# enable AddressSanitizer if supported
if (SDL_ASAN)
  asan_check_add_debug_flag2("address")
  asan_check_add_debug_flag("bool")
  asan_check_add_debug_flag("bounds")
  asan_check_add_debug_flag("enum")
  asan_check_add_debug_flag("float-cast-overflow")
  asan_check_add_debug_flag("float-divide-by-zero")
  asan_check_add_debug_flag("nonnull-attribute")
  asan_check_add_debug_flag("returns-nonnull-attribute")
  asan_check_add_debug_flag("signed-integer-overflow")
  asan_check_add_debug_flag("undefined")
  asan_check_add_debug_flag("vla-bound")
  asan_check_add_debug_flag("leak")
  # The object size sanitizer has no effect on unoptimized builds on Clang,
  # but causes warnings.
  if((NOT USE_CLANG) OR ("${CMAKE_BUILD_TYPE}" STREQUAL ""))
    asan_check_add_debug_flag("object-size")
  endif()
endif()

if(SDL_CCACHE AND NOT CMAKE_VERSION VERSION_LESS 3.4)
  cmake_minimum_required(VERSION 3.4...3.5)
  find_program(CCACHE_BINARY ccache)
  if(CCACHE_BINARY)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_BINARY})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_BINARY})
    set(CMAKE_OBJC_COMPILER_LAUNCHER ${CCACHE_BINARY})
    set(HAVE_CCACHE ON)
  else()
    set(HAVE_CCACHE OFF)
  endif()
else()
  set(HAVE_CCACHE OFF)
endif()

if(SDL_TESTS)
  set(HAVE_TESTS ON)
endif()

# Urho3D - remove custom target sdl_headers_copy for generated include files.

# Urho3D - restore the original CMake global settings
set (CMAKE_REQUIRED_FLAGS ${ORIG_CMAKE_REQUIRED_FLAGS})
set (CMAKE_REQUIRED_INCLUDES ${ORIG_CMAKE_REQUIRED_INCLUDES})
set (CMAKE_REQUIRED_LIBRARIES ${ORIG_CMAKE_REQUIRED_LIBRARIES})

##### Info output #####
message(STATUS "")
message(STATUS "SDL2 was configured with the following options:")
message(STATUS "")
message(STATUS "Platform: ${CMAKE_SYSTEM}")
message(STATUS "64-bit:   ${ARCH_64}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Revision: ${SDL_REVISION}")
message(STATUS "")
message(STATUS "Subsystems:")
foreach(_SUB ${SDL_SUBSYSTEMS})
  string(TOUPPER ${_SUB} _OPT)
  message_bool_option(${_SUB} SDL_${_OPT})
endforeach()
message(STATUS "")
message(STATUS "Options:")
list(SORT ALLOPTIONS)
foreach(_OPT ${ALLOPTIONS})
  # Get the padding
  string(LENGTH ${_OPT} _OPTLEN)
  math(EXPR _PADLEN "(${LONGESTOPTIONNAME} + 1) - ${_OPTLEN}")
  string(RANDOM LENGTH ${_PADLEN} ALPHABET " " _PADDING)
  message_tested_option(${_OPT} ${_PADDING})
endforeach()
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(STATUS "")
  message(STATUS " CMAKE_C_FLAGS_DEBUG:   ${CMAKE_C_FLAGS_DEBUG}")
  message(STATUS " CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()
message(STATUS "")
message(STATUS " CFLAGS:        ${CMAKE_C_FLAGS}")
message(STATUS " EXTRA_CFLAGS:  ${EXTRA_CFLAGS}")
message(STATUS " EXTRA_LDFLAGS: ${EXTRA_LDFLAGS} ${EXTRA_LDFLAGS_BUILD}")
message(STATUS " EXTRA_LIBS:    ${EXTRA_LIBS}")
message(STATUS "")
message(STATUS " Build Shared Library: ${SDL_SHARED}")
message(STATUS " Build Static Library: ${SDL_STATIC}")
if(SDL_STATIC)
    message(STATUS " Build Static Library with Position Independent Code: ${SDL_STATIC_PIC}")
endif()
message(STATUS "")
if(UNIX)
  message(STATUS "If something was not detected, although the libraries")
  message(STATUS "were installed, then make sure you have set the")
  message(STATUS "CFLAGS and LDFLAGS environment variables correctly.")
  message(STATUS "")
endif()

if(WARN_ABOUT_ARM_SIMD_ASM_MIT)
  message(STATUS "")
  message(STATUS "SDL is being built with ARM SIMD optimizations, which")
  message(STATUS "uses code licensed under the MIT license. If this is a")
  message(STATUS "problem, please disable that code by rerunning CMake with:")
  message(STATUS "")
  message(STATUS "    -DSDL_ARMSIMD=OFF")
endif()

if(WARN_ABOUT_ARM_NEON_ASM_MIT)
  message(STATUS "")
  message(STATUS "SDL is being built with ARM NEON optimizations, which")
  message(STATUS "uses code licensed under the MIT license. If this is a")
  message(STATUS "problem, please disable that code by rerunning CMake with:")
  message(STATUS "")
  message(STATUS "    -DSDL_ARMNEON=OFF")
endif()

# Ensure that the extra cflags are used at compile time
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} ${EXTRA_CFLAGS_BUILD}")

# Urho3D - Remove sdlmain

if(ANDROID)
  target_include_directories(sdl-build-options INTERFACE "${ANDROID_NDK}/sources/android/cpufeatures")
endif()

if(APPLE)
  cmake_push_check_state(RESET)
  check_c_compiler_flag(-fobjc-arc COMPILER_SUPPORTS_FOBJC_ARC)
  cmake_pop_check_state()
  if(NOT COMPILER_SUPPORTS_FOBJC_ARC)
    message(FATAL_ERROR "Compiler does not support -fobjc-arc: this is required on Apple platforms")
  endif()
  target_compile_options(sdl-build-options INTERFACE "-fobjc-arc")
endif()

if(PS2)
  target_compile_options(sdl-build-options INTERFACE "-Wno-error=declaration-after-statement")
endif()

if(APPLE)
  foreach(SOURCE_FILE ${SOURCE_FILES})
    get_filename_component(FILE_EXTENSION ${SOURCE_FILE} EXT)
    if(FILE_EXTENSION STREQUAL ".m")
      set_property(SOURCE ${SOURCE_FILE} APPEND_STRING PROPERTY COMPILE_FLAGS " -x objective-c")
    endif()
  endforeach()
endif()

# Urho3D - replace this part by Urho3D setup_library

##### Tests #####

##### Installation targets #####

##### Uninstall target #####

##### Tests subproject (must appear after the install/uninstall targets) #####

##### Fix Objective C builds #####
set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} ${CMAKE_C_FLAGS}")


######################################################
# Urho3D - anything below this line is specific
######################################################

# Define target name
set (TARGET_NAME SDL)

# Define source files
# Adding the headers into source files list is just for easier file browsing in the IDE
file (GLOB H_FILES include/*.h)
list (APPEND SOURCE_FILES ${H_FILES})

# Add Main sources
if (IOS OR TVOS OR UWP OR ANDROID)
  list (APPEND SOURCE_FILES ${SDLMAIN_SOURCES})
endif ()

# Setup target as STATIC library (as the result we never use EXTRA_LDFLAGS linker flags)
setup_library ()

# This picks up all the compiler options and such we've accumulated up to here.
target_link_libraries(SDL $<BUILD_INTERFACE:sdl-build-options>)

# Install headers for building and using the Urho3D library (install dependency for InputEvents.h)
install_header_files (DIRECTORY include/ DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/SDL FILES_MATCHING PATTERN *.h USE_FILE_SYMLINK)    # Note: the trailing slash is significant
install_header_files (FILES ${SDL2_BINARY_DIR}/include/generated/SDL_config.h DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/SDL)

