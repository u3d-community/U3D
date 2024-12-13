<div align="center">
<a href="https://u3d.io">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/u3d-community/U3D/master/SourceAssets/LogoDark.svg">
    <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/u3d-community/U3D/master/SourceAssets/Logo.svg">
    <img width="200" alt="U3D Logo" src="https://raw.githubusercontent.com/u3d-community/U3D/master/SourceAssets/Logo.svg">
  </picture>
</a>

# U3D
[![android](https://github.com/u3d-community/U3D/actions/workflows/android.yml/badge.svg?branch=master)](https://github.com/u3d-community/U3D/actions/workflows/android.yml)
[![ios](https://github.com/u3d-community/U3D/actions/workflows/ios.yml/badge.svg?branch=master)](https://github.com/u3d-community/U3D/actions/workflows/ios.yml)
[![linux](https://github.com/u3d-community/U3D/actions/workflows/linux.yml/badge.svg?branch=master)](https://github.com/u3d-community/U3D/actions/workflows/linux.yml)
[![macos](https://github.com/u3d-community/U3D/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/u3d-community/U3D/actions/workflows/macos.yml)
[![windows](https://github.com/u3d-community/U3D/actions/workflows/windows.yml/badge.svg?branch=master)](https://github.com/u3d-community/U3D/actions/workflows/windows.yml)
![GitHub](https://img.shields.io/github/license/u3d-community/U3D)
[![Discord](https://img.shields.io/discord/1044444861992026192)](https://discord.gg/httHCqcXGx)
</div>

**U3D** is a open source, lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Forked from [Urho3D](https://web.archive.org/web/20170406232054/https://urho3d.github.io/) and greatly inspired by OGRE and Horde3D.

Main website: [https://u3d.io/](https://u3d.io/)

## License
Licensed under the MIT license, see [LICENSE](https://github.com/u3d-community/U3D/blob/master/LICENSE) for details.

## Features
![U3D Screenshot](https://user-images.githubusercontent.com/467729/204386186-fbc2b8f8-cff8-4549-9fbc-5a6d35869a08.png)

- Cross-platform support: Windows, Linux, macOS, iOS, tvOS, Android, Raspberry Pi (and other generic ARM boards), and Web with Emscripten
- Powerful and configurable rendering pipeline, with implementations for forward, light pre-pass and deferred rendering
- OpenGL, OpenGL ES, WebGL and Direct3D11
- All-in-one editor built with ActionScript, allowing you to easily customize the editor to your game needs
- Two scripting languages available for creating game logic: Lua and AngelScript
- UI, localization and database subsystems
- Component based scene model
- Skeletal, vertex morph and node animation
- Point, spot and directional lights
- Cascaded shadow maps
- Particle rendering
- Geomipmapped terrain
- Static and skinned decals
- LODs
- Automatic instancing
- Software rasterized occlusion culling
- Post-processing
- HDR and PBR rendering
- 2D sprites and particles that integrate into the 3D scene
- Task-based multithreading
- Hierarchical performance profiler
- Scene, object and prefab load/save in binary, XML and JSON formats
- Keyframe animation of object attributes
- Background loading of resources
- Keyboard, mouse, joystick and touch input
- High-performance math library
- Physics powered by Bullet and Box2D
- Networking powered by SLikeNet
- Pathfinding and Crowd Simulation via Recast/Detour
- Supported images: JPEG, PNG, TGA, BMP, PSD, GIF, HDR
- Compressed image support: DDS, KTX, PVR
- Import virtually any 3D file format via [Assimp](https://github.com/assimp/assimp/blob/master/doc/Fileformats.md)
- 2D and 3D audio playblack, Ogg Vorbis and WAV support
- TrueType font rendering
- Built as a single external library, can be linked statically or dynamically

## Design Goals
- **Productive**: Do not reinvent the wheel. Engine should reduce development time. Less yak shaving, more game making.
- **Fast**: Compile and run fast. Parallelize when possible.
- **Modular**: Only use what you need. Flexibility for power users.

## Conventions
U3D uses the following conventions and principles:

- Left-handed coordinates. Positive X, Y & Z axes point to the right, up, and forward, and positive rotation is clockwise.
- Degrees are used for angles.
- Clockwise vertices define a front face.
- Audio volume is specified from 0.0 (silence) to 1.0 (full volume)
- Path names use slash instead of backslash. Paths will be converted internally into the necessary format when calling into the operating system.
- In the script API, properties are used whenever appropriate instead of Set...() and Get...() functions. If the setter and getter require index parameters, the property will use array-style indexing, and its name will be in plural. For example model->SetMaterial(0, myMaterial) in C++ would become model.materials[0] = myMaterial in script.
- Raw pointers are used whenever possible in the classes' public API. This simplifies exposing functions & classes to script, and is relatively safe, because `SharedPtr` & `WeakPtr` use intrusive reference counting.
- When an object's public API allows assigning a reference counted object to it through a Set...() function, this implies ownership through a `SharedPtr`. For example assigning a Material to a StaticModel, or a Viewport to Renderer. To end the assignment and free the reference counted object, call the Set...() function again with a null argument.
- No C++ exceptions. Error return values (false / null pointer / dummy reference) are used instead. Script exceptions are used when there is no other sensible way, such as with out of bounds array access.
- Feeding illegal data to public API functions, such as out of bounds indices or null pointers, should not cause crashes or corruption. Instead errors are logged as appropriate.
- Third party libraries are included as source code for the build process. They are however hidden from the public API as completely as possible.

## Community
U3D's development is community-driven and completely independent, empowering developers to build and design better tools to help indie game development. We invite you to familiarize yourself with our [**Code of Conduct**](./CODE_OF_CONDUCT.md) and get to know us on:

- **[Discord](https://discord.gg/httHCqcXGx)**: Community chat room
- **[GitHub Discussions](https://github.com/u3d-community/U3D/discussions)**: Technical discussions about the roadmap, issues, bugs

If you'd like to help build U3D, we have a guide just for you! Check it out: **[Contributor Guide](./CONTRIBUTING.md)**.

## Credits
U3D is greatly inspired by [Urho3D](https://web.archive.org/web/20170406232054/https://urho3d.github.io/), [OGRE](http://www.ogre3d.org) and [Horde3D](http://www.horde3d.org). Additional inspiration & research used:

- Rectangle packing by Jukka Jylänki (clb)
  http://clb.demon.fi/projects/rectangle-bin-packing
- Tangent generation from Terathon
  http://www.terathon.com/code/tangent.html
- Fast, Minimum Storage Ray/Triangle Intersection by Möller & Trumbore
  http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
- Linear-Speed Vertex Cache Optimisation by Tom Forsyth
  http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
- Software rasterization of triangles based on Chris Hecker's
  Perspective Texture Mapping series in the Game Developer magazine
  http://chrishecker.com/Miscellaneous_Technical_Articles
- Networked Physics by Glenn Fiedler
  http://gafferongames.com/game-physics/networked-physics/
- Euler Angle Formulas by David Eberly
  https://www.geometrictools.com/Documentation/EulerAngles.pdf
- Red Black Trees by Julienne Walker
  http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
- Comparison of several sorting algorithms by Juha Nieminen
  http://warp.povusers.org/SortComparison/

U3D uses the following third-party libraries:

- AngelScript 2.35.1 WIP (http://www.angelcode.com/angelscript)
- Boost 1.64.0 (http://www.boost.org) - only used for AngelScript generic bindings
- Box2D 2.3.2 WIP (http://box2d.org)
- Bullet 3.06+ (http://www.bulletphysics.org)
- Civetweb 1.7 (https://github.com/civetweb/civetweb)
- FreeType 2.8 (https://www.freetype.org)
- {fmt} 9.1.0 (https://fmt.dev/)
- GLEW 1.13.0 (http://glew.sourceforge.net)
- SLikeNet (https://github.com/SLikeSoft/SLikeNet)
- libcpuid 0.4.0+ (https://github.com/anrieff/libcpuid)
- Lua 5.1 (https://www.lua.org)
- LuaJIT 2.1.0+ (http://www.luajit.org)
- LZ4 1.7.5 (https://github.com/lz4/lz4)
- MojoShader (https://icculus.org/mojoshader)
- Mustache 1.0 (https://mustache.github.io, https://github.com/kainjow/Mustache)
- nanodbc 2.12.4+ (https://lexicalunit.github.io/nanodbc)
- Open Asset Import Library 5.4.3 (http://assimp.sourceforge.net)
- pugixml 1.13 (http://pugixml.org)
- rapidjson 1.1.0 commit 2024/12/10 (https://github.com/Tencent/rapidjson/commit/58c6938b73c8685d82905ed55ec5b59e8f163687)
- Recast/Detour (https://github.com/recastnavigation/recastnavigation)
- SDL 2.30.9 (https://www.libsdl.org)
- SQLite 3.36.0 (https://www.sqlite.org)
- StanHull (https://codesuppository.blogspot.com/2006/03/john-ratcliffs-code-suppository-blog.html)
- stb_image 2.29 (https://nothings.org)
- stb_image_write 1.16 (https://nothings.org)
- stb_rect_pack 1.01 (https://nothings.org)
- stb_vorbis 1.22 (https://nothings.org)
- tolua++ 1.0.93 (defunct - http://www.codenix.com/~tolua)
- WebP (https://chromium.googlesource.com/webm/libwebp)
- ETCPACK (https://github.com/Ericsson/ETCPACK)
- Tracy 0.7.6 (https://github.com/wolfpld/tracy)
- DXT / PVRTC decompression code based on the Squish library and the Oolong Engine.

### Media credits
- Jack and mushroom models from the realXtend project. (https://www.realxtend.org)
- Ninja model and terrain, water, smoke, flare and status bar textures from OGRE.
- BlueHighway font from Larabie Fonts.
- Anonymous Pro font by Mark Simonson.
- NinjaSnowWar sounds by Veli-Pekka Tätilä.
- PBR textures from Substance Share. (https://share.allegorithmic.com)
- IBL textures from HDRLab's sIBL Archive.
- Dieselpunk Moto model by allexandr007.
- Mutant & Kachujin models from Mixamo.
- Skies from Polyhaven. (https://polyhaven.com/license)

License / copyright information included with the assets as necessary. All other assets (including shaders) by U3D authors and licensed similarly as the engine itself.

### Contributors
<a href="https://github.com/u3d-community/U3D/graphs/contributors"><img src="https://opencollective.com/U3D/contributors.svg?width=890&button=false" /></a>
