# Contributing to U3D
So you want to contribute to U3D? You are in the right place. We're happy that you have chosen to help and we want you, and everyone else, to feel welcome. To start, make sure you are familiar with our community's [Code of Conduct](https://github.com/u3d-community/U3D/blob/master/CODE_OF_CONDUCT.md) and say hi on [Discord](https://discord.gg/httHCqcXGx).

## Checklist
When contributing code to the U3D project, there are a few things to check so that the process goes smoothly.

First of all, the contribution should be wholly your own, so that you hold the copyright. If you are borrowing anything (for example a specific implementation of a math formula), you must be sure that you're allowed to do so, and give credit appropriately. For example borrowing code that is in the public domain would be OK.

Second, you need to agree that code is released under the MIT license with the copyright statement "Copyright (c) 2022-2023 the U3D project." Note here that "U3D project" is not an actual legal entity, but just shorthand to avoid listing all the contributors. You certainly retain your individual copyright. You should copy-paste the license statement from an existing .cpp or .h file to each new file that you contribute, to avoid having to add it later.

Third, there are requirements for new code that come from U3D striving to be a cohesive, easy-to-use package where features like events, serialization and script bindings integrate tightly. Check all that apply:

- For all code (classes, functions) for which it makes sense, both AngelScript and Lua bindings should exist. Refer to the existing bindings and the scripting documentation for specific conventions, for example the use of properties in AngelScript instead of setters / getters where possible, or Lua bindings providing both functions and properties.
- Script bindings do not need to be made for low-level functionality that only makes sense to be called from C++, such as thread handling or byte-level access to GPU resources, or for internal but public functions that are only accessed by subsystems and are not needed when using the classes.
- Unless impossible due to missing bindings (see above) new examples should be implemented in all of C++, AngelScript and Lua.
- For new Component or UIElement subclasses,  attributes should exist for serialization, network replication and editing. The classes should be possible to create from within the editor; check the component category and supply icons for them as necessary (see the files bin/Data/Textures/EditorIcons.png and bin/Data/UI/EditorIcons.xml.)
- If the classes inherit attribute definitions from other classes, make sure that they are registered in the correct order on engine initialization.
- Network replication of a component's attributes must be triggered manually by calling MarkNetworkUpdate() in each setter function that modifies a network-replicated attribute. See the Camera class for a straightforward example.
- Define  events where you anticipate that an external party may want to hook up to something happening. For example the editor updates its scene hierarchy window by subscribing to the scene change events: child node added/removed, component added/removed.
- Mark all application-usable classes, structs and functions with the macro URHO3D_API so that they are exported correctly in a shared library build.
- Please heed all the  coding conventions and study existing code where unsure. Ensure that your text editor or IDE is configured to show whitespace, so that you don't accidentally mix spaces and tabs.
- Whenever you are creating a major new feature, its usage should be documented in the .dox pages in addition to the function documentation in the header files. Create a new page if necessary, and link from an appropriate place in the existing documentation. If it's for example a new rendering feature, it could be linked from the  Rendering page. If the feature introduces a completely new subsystem, it should be linked from the main page list in U3D.dox.
- When you add a new sample application, it should be implemented in all of C++, AngelScript and Lua if possible, to show how all of the APIs are used. Check sample numbering for the next free one available. If a (C++) sample depends on a specific U3D build option, for example physics or networking, it can be conditionally disabled in its CMakeLists.txt. See 11_Physics sample for an example.

## Considerations when submitting a pull request
- Make sure the PR description clearly describes the problem and solution. If the PR is fixing an existing issue, please include `Fix #123` in the body.
- Don't submit big pull requests (changing tons of files), they are difficult to review. It's better to send small pull requests, one at a time.

## Third party library considerations
- When you add a new third-party library, insert its license statement to the LICENSE in the Source\ThirdParty directory. Only libraries with permissive licenses such as BSD/MIT/zlib are accepted, because complying for example with the LGPL is difficult on mobile platforms, and would leave the application in a legal grey area.
- Prefer small and well-focused libraries for the U3D runtime. For example we use stb_image instead of FreeImage to load images, as it's assumed that the application developer can control the data and do offline conversion to supported formats as necessary.
- Third-party libraries should not leak C++ exceptions or use of STL containers into the U3D public API. Do a wrapping for example on the subsystem level if necessary.

## Engine Architecture
The U3D engine compiles into one library. Conceptually it consists of several "sublibraries" that represent different subsystems or functionality. Each of these resides in a subdirectory under the `Source/U3D` directory:

- *Container*: Provides STL replacement classes and shared pointers.
- *Math*: Provides vector, quaternion & matrix types and geometric shapes used in intersection tests.
- *Core*: Provides the execution `Context`, the base class `Object` for typed objects, object factories, event handling, threading and profiling.
- *IO*: Provides file system access, stream input/output and logging.
- *Resource*: Provides the `ResourceCache` and the base resource types, including XML documents.
- *Scene*: Provides `Node` and `Component` classes, from which U3D scenes are built.
- *Graphics*: Provides application window handling and 3D rendering capabilities.
- *Input*: Provides input device access in both polled and event-based mode.
- *Network*: Provides client-server networking functionality.
- *Audio*: Provides the audio subsystem and playback of .wav & .ogg sounds in either 2D or 3D.
- *UI*: Provides graphical user interface elements.
- *Physics*: Provides physics simulation.
- *Navigation*: Provides navigation mesh generation and pathfinding.
- *Urho2D*: Provides 2D rendering components that integrate into the 3D scene.
- *Script*: Provides scripting support using the AngelScript language.
- *Engine*: Instantiates the subsystems from the modules above (except Script, which needs to be instantiated by the application) and manages the main loop iteration.

## Execution context
The heart of U3D is the `Context` object, which must always be created as the first in a U3D application, and deleted last. All "important" objects that derive from the `Object` base class, such as scene nodes, resources like textures and models, and the subsystems themselves require `Context` pointer in their constructor. This avoids both the singleton pattern for subsystems, or having to pass around several objects into constructors.

The `Context` provides the following functionality (described in detail on their own pages):

- Registering and accessing subsystems.
- Creation and reflection facilities per object type: object factories and serializable attributes.
- Sending events between objects.


Thanks for reading and helping make U3D a great project,
:heart: The U3D Team
