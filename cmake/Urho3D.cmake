#
# Copyright (c) 2022-2024 the U3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Input variables:
#
# URHO3D_HOME: Urho3D home directory.
# URHO3D_HOME should be a build directory (e.g., URHO3D_BUILD_DIR) that contains subfolders "include" and "lib" if Urho3D is used as an SDK/build library.
# URHO3D_HOME should be the root directory (e.g., URHO3D_ROOT_DIR) that contains all the content of Urho3D if it is used as a submodule (source code).
# URHO3D_ROOT_DIR, URHO3D_SOURCE_DIR, and URHO3D_BUILD_DIR are defined to handle each specific case depending on how Urho3D is used.

# Output variables:
#
# URHO3D_AS_SUBMODULE: Boolean, true if Urho3D is used in the user project as a submodule (i.e., "as third-party, from source").
# URHO3D_ROOT_DIR: The path containing all Urho3D bin/source/CMake folders.
# URHO3D_SOURCE_DIR: The path to the Urho3D source.
# URHO3D_BUILD_DIR: A specific Urho3D build directory (SDK or build tree) that contains the Urho3D include files and library (.a, .lib, .so, or .dll).
#                    If Urho3D is used as a submodule, it will be built in ${CMAKE_BINARY_DIR}/urho3d.
# URHO3D_CMAKE_MODULE: A specific directory containing UrhoCommon.cmake.
#                      This allows using a custom set of CMake files or the latest available with older Urho3D sources.

# TODO: Add an option to download Urho3D source/SDK from Git (use the CMake FetchContent module?).
# TODO: Patcher to finish
# TODO: finish the tag list generation in discover module
# TODO: allow select crosscompiled build
# TODO: remove android quick fix

string (TOUPPER ${CMAKE_PROJECT_NAME} PROJECTNAME)

# Find the Urho3D root path and source path from a presumed Urho3D subfolder.
function (urho_find_origin dir root source origin)
    unset (${root} PARENT_SCOPE)
    unset (${source} PARENT_SCOPE)
    unset (${origin} PARENT_SCOPE)
    set (currentpath "${dir}")
    while (NOT urhoroot AND currentpath AND NOT previouspath STREQUAL currentpath)
        if (EXISTS "${currentpath}/Source" AND EXISTS "${currentpath}/README.md") # Detect Urho3D source distribution           
            set (${origin} "source" PARENT_SCOPE)
            set (${root}   "${currentpath}" PARENT_SCOPE)
            set (${source} "${currentpath}/Source" PARENT_SCOPE)
            break ()
        endif ()
        if (EXISTS "${currentpath}/lib" OR EXISTS "${currentpath}/lib64")
            if (EXISTS "${currentpath}/share/Urho3D/cmake/Modules/UrhoCommon.cmake" OR  
                EXISTS "${currentpath}/share/cmake/Modules/UrhoCommon.cmake")     # Detect Urho3D SDK
                set (${origin} "sdk" PARENT_SCOPE)
                set (${root}   "${currentpath}" PARENT_SCOPE)
                break ()
            endif ()
            if (EXISTS "${currentpath}/include/Urho3D/Urho3DAll.h" AND 
                EXISTS "${currentpath}/CMakeCache.txt")                         # Detect Urho3D build tree
                set (${origin} "build" PARENT_SCOPE)
                set (${root}   "${currentpath}" PARENT_SCOPE)
                break ()
            endif ()            
        endif ()
        message ("urho_find_origin : path = ${currentpath}")
        set (previouspath ${currentpath})
        get_filename_component (currentpath ${previouspath} DIRECTORY)
    endwhile ()
endfunction ()

function (urho_cmake_module dir module_dir)
    set (dir "${CMAKE_CURRENT_SOURCE_DIR}")
    while (NOT dir STREQUAL "/")
        if (EXISTS "${dir}/cmake/Modules/UrhoCommon.cmake")
            set (${module_dir} "${dir}/cmake/Modules" PARENT_SCOPE)
            break ()
        endif ()
        get_filename_component (dir ${dir} DIRECTORY)
    endwhile ()
endfunction ()

# Hide common Urho3D options that are not needed when Urho3D is used as a pre-built library.
macro (urho_mark_as_advanced_options)
    # These are Urho3D build options that are never modified by a user project when using the Urho3D SDK or build tree.
    # TODO: Use a "script/.build-options" file?
    set (URHO3D_BUILDOPTIONS URHO3D_ANGELSCRIPT URHO3D_FORCE_AS_MAX_PORTABILITY URHO3D_DATABASE URHO3D_FILEWATCHER URHO3D_IK URHO3D_LOGGING URHO3D_LUA 
                             URHO3D_NAVIGATION URHO3D_NETWORK URHO3D_PHYSICS URHO3D_PHYSICS2D URHO3D_URHO2D URHO3D_PROFILING URHO3D_TRACY_PROFILING
                             URHO3D_THREADING URHO3D_VMA URHO3D_VOLK URHO3D_VULKAN_VALIDATION URHO3D_SPIRV URHO3D_WEBP URHO3D_WIN32_CONSOLE URHO3D_MINIDUMPS)
    foreach (VAR ${URHO3D_BUILDOPTIONS})
        mark_as_advanced (FORCE ${VAR})
    endforeach ()
endmacro ()

if (EXISTS ${CMAKE_SOURCE_DIR}/cmake)
    set (PROJECT_CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
else()
    message ("Cannot find the cmake directory!")
    return ()
endif ()

# Check URHO3D_HOME
if (URHO3D_HOME AND NOT EXISTS ${URHO3D_HOME})
    unset (URHO3D_HOME)
endif ()

# Add UrhoDiscover
if (EXISTS "${PROJECT_CMAKE_DIR}/UrhoDiscover.cmake")
    include (${PROJECT_CMAKE_DIR}/UrhoDiscover.cmake)
endif ()

# Get UrhoPatcher
if (EXISTS "${PROJECT_CMAKE_DIR}/UrhoPatcher.cmake")
    set (URHO3D_PATCHER_EXISTS TRUE)
else ()
    set (URHO3D_PATCHER_EXISTS FALSE)
endif ()

# Apply Patcher method : remove previously patched CMake files, restoring the originals.
if (URHO3D_PATCHER_EXISTS)
    set (URHO3D_PATCHER_RESTORE TRUE)
    include (${PROJECT_CMAKE_DIR}/UrhoPatcher.cmake)
    if (NOT URHO3D_PATCHER_SUCCESS)
        set (URHO3D_HOME "")
        return ()
    endif() 
endif ()

# Stop here if URHO3D_HOME is empty or undefined.
# URHO3D_HOME should be set manually.
if (NOT URHO3D_HOME AND NOT ANDROID) # TODO : android quick fix
    if (${PROJECTNAME}_URHO3D_DIRS)
        message ("URHO3D_DISCOVER has found some Urho3D folders. Please select one with cmake-gui.")
    else ()
        message ("URHO3D_HOME is undefined!")
    endif ()
    return ()
endif ()

# At this point, URHO3D_HOME is defined.
# Define the following variables: URHO3D_ROOT_DIR, URHO3D_SOURCE_DIR, URHO3D_BUILD_DIR, URHO3D_AS_SUBMODULE, URHO3D_CMAKE_MODULE.
# These variables ensure separation between the Urho3D build and the user project.
unset (URHO3D_ROOT_DIR)
unset (URHO3D_SOURCE_DIR)
unset (URHO3D_BUILD_DIR)
set (URHO3D_AS_SUBMODULE FALSE CACHE INTERNAL BOOLEAN)

# Determine the origin of Urho3D based on URHO3D_HOME.
# Example: Retrieve URHO3D_ROOT_DIR="/path/to/urho3d/urho3d-1.x/" from URHO3D_HOME="/path/to/urho3d/urho3d-1.x/build/linux/static/release".
unset (origin)

if (NOT ANDROID) # TODO : android quick fix
    urho_find_origin ("${URHO3D_HOME}" URHO3D_ROOT_DIR URHO3D_SOURCE_DIR origin)
    if (NOT origin)
        message (FATAL_ERROR "The Urho3D path appears to be invalid!")
    endif ()
endif ()

if (origin STREQUAL "source")
    set (URHO3D_AS_SUBMODULE TRUE CACHE INTERNAL BOOLEAN)
    set (URHO3D_BUILD_DIR ${CMAKE_BINARY_DIR}/urho3d)
endif ()

# Set a user-provided CMake module path or use the default path.
if (${PROJECTNAME}_URHO3D_CMAKE_MODULE AND NOT "${${PROJECTNAME}_URHO3D_CMAKE_MODULE}" STREQUAL "${URHO3D_CMAKE_MODULE}")
    set (URHO3D_CMAKE_MODULE "${${PROJECTNAME}_URHO3D_CMAKE_MODULE}")
endif ()
if (NOT URHO3D_CMAKE_MODULE)
    if (EXISTS "${PROJECT_CMAKE_DIR}/Modules/UrhoCommon.cmake")
        set (URHO3D_CMAKE_MODULE ${PROJECT_CMAKE_DIR}/Modules)
    elseif (EXISTS "${URHO3D_ROOT_DIR}/cmake/Modules/UrhoCommon.cmake")
        set (URHO3D_CMAKE_MODULE ${URHO3D_ROOT_DIR}/cmake/Modules)
    endif ()
endif ()
if (URHO3D_CMAKE_MODULE)
    set (URHO3D_CMAKE_MODULE ${URHO3D_CMAKE_MODULE} CACHE INTERNAL STRING)
    set (${PROJECTNAME}_URHO3D_CMAKE_MODULE ${URHO3D_CMAKE_MODULE} CACHE PATH "Path to Urho3D CMake modules for all Urho3D sources" FORCE)
endif ()

list (REMOVE_ITEM CMAKE_MODULE_PATH ${URHO3D_CMAKE_MODULE})
list (PREPEND CMAKE_MODULE_PATH ${URHO3D_CMAKE_MODULE})

if (origin)
    # Apply patches if enabled.
    if (URHO3D_PATCHER_EXISTS)
        set (URHO3D_PATCHER_APPLY TRUE)
        include (${PROJECT_CMAKE_DIR}/UrhoPatcher.cmake)
        if (NOT URHO3D_PATCHER_SUCCESS)
            set (URHO3D_HOME "")
            return ()
        endif()        
    endif ()

    message (" -- from ${origin}")
    message (" -- Using URHO3D_HOME: ${URHO3D_HOME}")
    message (" -- Using CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

    # Include Urho3D sources (if used as a submodule) or call UrhoCommon (if used as an SDK/build tree).
    if (URHO3D_AS_SUBMODULE)
        # Add Urho3D sources and set the build directory to URHO3D_BUILD_DIR.
        add_subdirectory (${URHO3D_ROOT_DIR} ${URHO3D_BUILD_DIR})
    endif ()
    
    if (NOT URHOCOMMON_INUSE)
        # Include UrhoCommon for the main user project if not already used
        include (${URHO3D_CMAKE_MODULE}/UrhoCommon.cmake)
    endif ()
endif ()

# Project install prefix path.
set (${PROJECTNAME}_INSTALL_PREFIX "" CACHE STRING "${CMAKE_PROJECT_NAME} install prefix added to the global CMake install prefix path.")
