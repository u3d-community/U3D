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

# Find all Urho3D sources or libraries (SDKs or build trees) available on the device.
# Only use this in a user project CMake file.

# Input variables:
#
# URHO3D_SEARCH_PATH  : The root path where Urho3D libraries will be searched.
#                       If the list is empty, the current directory will be added.
# FORCEDISCOVER : Force the search

# Added cached variables:
#
# ${PROJECTNAME}_URHO3D_DIRS : List of all discovered Urho3D source or build paths.
# ${PROJECTNAME}_URHO3D_TAGS : A UI-friendly list of URHO3D_DIRS.
# ${PROJECTNAME}_URHO3D_SELECT : The selected Urho3D directory.

# Output variable:
#
# URHO3D_HOME

# currently, cross-compiled SDK/builds are excluded.
set (EXCLUDE_CROSSBUILD TRUE)

function (get_build_target dir device)
    unset (${device} PARENT_SCOPE)
    # TODO: get compile option for a SDK?
    # get the compile option for a build tree.
    if (EXISTS ${dir}/CMakeCache.txt)
        file (STRINGS ${dir}/CMakeCache.txt HeaderStrings)       
        foreach (VAR ${HeaderStrings})
            string (REPLACE ";" " " VAR ${VAR})
            unset (match)
            string (REGEX MATCH "^RPI_ABI:STRING=(.*)$" match ${VAR})
            if (match)
                set (targetdevice "RPI_${CMAKE_MATCH_1}")
            elseif (${VAR} MATCHES "^MINGW_PREFIX:STRING")
                set (targetdevice "MINGW")
            elseif (${VAR} MATCHES "^EMSCRIPTEN_SYSROOT:")
                set (targetdevice "WEB")
            elseif (${VAR} MATCHES "^IOS:")
                set (targetdevice "IOS")
            elseif (${VAR} MATCHES "^TVOS:")
                set (targetdevice "TVOS") 
            elseif (${VAR} MATCHES "^ARM_PREFIX:STRING")
                set (targetdevice "ARM")                                                
            endif ()                                                                          
            if (targetdevice)
                break ()
            endif ()
        endforeach ()
    endif ()
    if (targetdevice)
        set (${device} ${targetdevice} PARENT_SCOPE)
    endif ()
endfunction()

function (get_build_arch dir archtype)
    unset (${archtype} PARENT_SCOPE)
    # TODO: get compile option for a SDK?
    # get the compile option for a build tree.
    if (EXISTS ${dir}/CMakeCache.txt)
        file (STRINGS ${dir}/CMakeCache.txt HeaderStrings)       
        foreach (VAR ${HeaderStrings})
            string (REPLACE ";" " " VAR ${VAR})
            if (${VAR} STREQUAL "URHO3D_64BIT:BOOL=ON")
                set (archtype "64BIT" PARENT_SCOPE)
                break ()
            elseif (${VAR} STREQUAL "URHO3D_64BIT:BOOL=OFF") 
                set (archtype "32BIT" PARENT_SCOPE)
                break ()
            endif ()           
        endforeach()
    endif ()  
endfunction ()

function (get_build_libtype dir libtype)
    unset (${libtype} PARENT_SCOPE)
    # TODO: get compile option for a SDK?
    # get the compile option for a build tree.
    if (EXISTS ${dir}/CMakeCache.txt)
        file (STRINGS ${dir}/CMakeCache.txt HeaderStrings)       
        foreach (VAR ${HeaderStrings})
            string (REPLACE ";" " " VAR ${VAR})
            unset (match)
            string (REGEX MATCH "^URHO3D_LIB_TYPE:STRING=(.*)$" match ${VAR})
            if (match)
                set (${libtype} "${CMAKE_MATCH_1}" PARENT_SCOPE)
                break ()
            endif ()            
        endforeach()
    endif ()  
endfunction ()

function (get_build_buildtype dir buildtype)
    unset (${buildtype} PARENT_SCOPE)
    # TODO: get compile option for a SDK?
    # get the compile option for a build tree.
    if (EXISTS ${dir}/CMakeCache.txt)
        file (STRINGS ${dir}/CMakeCache.txt HeaderStrings)       
        foreach (VAR ${HeaderStrings})
            string (REPLACE ";" " " VAR ${VAR})
            unset (match)
            string (REGEX MATCH "^CMAKE_BUILD_TYPE:STRING=(.*)$" match ${VAR})
            if (match)
                set (${buildtype} "${CMAKE_MATCH_1}" PARENT_SCOPE)
                break ()
            endif ()            
        endforeach()
    endif ()  
endfunction ()

# retrieve Urho3D revision.
# only works with Git (not applicable for manual installations from zip files).
function (urho_get_revision urhoroot revision)
    execute_process (COMMAND git describe --dirty WORKING_DIRECTORY ${urhoroot} RESULT_VARIABLE GIT_EXIT_CODE OUTPUT_VARIABLE LIB_REVISION ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT GIT_EXIT_CODE EQUAL 0)
        set (LIB_REVISION Unversioned)
    endif ()
    set (${revision} ${LIB_REVISION} PARENT_SCOPE)
endfunction ()

# Search in ${PROJECTNAME}_URHO3D_SEARCH_PATH for all occurrences of Source/Urho3D/CMakeLists.txt
macro (urho_find_sources_dirs)
    if (MSVC)
        execute_process (
            COMMAND powershell -Command "Get-ChildItem -Path '${${PROJECTNAME}_URHO3D_SEARCH_PATH}' -Recurse -Filter 'CMakeLists.txt' | 
                                            Where-Object { $_.DirectoryName -like \"*Source\\Urho3D*\" } | 
                                            Select-Object FullName"
            RESULT_VARIABLE PS_RESULT
            ERROR_VARIABLE FIND_ERRORS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE HEADERS
        )
        if(DEFINED ENV{URHO3D_HOME})
            execute_process (
                COMMAND powershell -Command "Get-ChildItem -Path '$ENV{URHO3D_HOME}' -Recurse -Filter 'CMakeLists.txt' | 
                                            Where-Object { $_.DirectoryName -like \"*Source\\Urho3D*\" } | 
                                            Select-Object FullName"
                RESULT_VARIABLE PS_RESULT
                ERROR_VARIABLE FIND_ERRORS
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE ENV_HEADERS
            )
        endif()
    else ()
        execute_process (
            COMMAND find "${${PROJECTNAME}_URHO3D_SEARCH_PATH}" -name "CMakeLists.txt"
                ! -path "*/.*" ! -path "*/android/*" ! -path "*/bin*/*" ! -path "*/build*/*" ! -path "*/cmake/*"
                ! -path "*/CMake/*" ! -path "*/Docs/*" ! -path "*/include/*" ! -path "*/gradle/*" ! -path "*/script/*"
                ! -path "*/source/*" ! -path "*/SourceAssets/*" ! -path "*/tools/*"
                ! -path "*/website/*" ! -path "*/CMakeFiles/*" ! -path "*/generated/*" ! -path "*/lib/*"
            COMMAND grep "/Source/Urho3D/CMakeLists.txt$"
            ERROR_VARIABLE FIND_ERRORS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE HEADERS
        )
        if(DEFINED ENV{URHO3D_HOME})
            execute_process (
                COMMAND find "$ENV{URHO3D_HOME}" -name "CMakeLists.txt"
                    ! -path "*/.*" ! -path "*/android/*" ! -path "*/bin*/*" ! -path "*/build*/*" ! -path "*/cmake/*"
                    ! -path "*/CMake/*" ! -path "*/Docs/*" ! -path "*/include/*" ! -path "*/gradle/*" ! -path "*/script/*"
                    ! -path "*/source/*" ! -path "*/SourceAssets/*" ! -path "*/tools/*"
                    ! -path "*/website/*" ! -path "*/CMakeFiles/*" ! -path "*/generated/*" ! -path "*/lib/*"
                COMMAND grep "/Source/Urho3D/CMakeLists.txt$"
                ERROR_VARIABLE FIND_ERRORS
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE ENV_HEADERS
            )
        endif()
    endif ()
    if (FIND_ERRORS)
        message ("urho_find_sources_dirs ERROR: ${FIND_ERRORS}")
        return ()
    endif ()
    if(ENV_HEADERS)
        string(JOIN "\n" HEADERS "${HEADERS}" "${ENV_HEADERS}")
    endif()
    set (NUM_SOURCE_DIRS 0)
    if (HEADERS)
        string (REPLACE "\n" ";" HEADERS_LIST "${HEADERS}")
        foreach (HEADER ${HEADERS_LIST})
            if (HEADER MATCHES "CMakeLists.txt$")
                unset (DIR)
                file (TO_CMAKE_PATH "${HEADER}" DIR)
                string (REPLACE "/Source/Urho3D/CMakeLists.txt" "" DIR ${DIR})
                list (APPEND ${PROJECTNAME}_URHO3D_DIRS ${DIR})
                math (EXPR NUM_SOURCE_DIRS "${NUM_SOURCE_DIRS} + 1")
                message (" ... source: ${DIR}")
            endif ()
        endforeach ()
    endif ()        
    message ("urho_find_sources_dirs NUM_SOURCE_DIRS = ${NUM_SOURCE_DIRS}")
endmacro ()

# Search in ${PROJECTNAME}_URHO3D_SEARCH_PATH for all occurrences of include/Urho3D/Urho3D.h
macro (urho_find_builds_dirs)
    if (MSVC)
        execute_process (
            COMMAND powershell -Command "Get-ChildItem -Path '${${PROJECTNAME}_URHO3D_SEARCH_PATH}' -Recurse -Filter 'Urho3D.h' | 
                                            Where-Object { $_.DirectoryName -like \"*include\\Urho3D*\" } | 
                                            Select-Object FullName"
            RESULT_VARIABLE PS_RESULT
            ERROR_VARIABLE FIND_ERRORS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE HEADERS
        )
        if(DEFINED ENV{URHO3D_HOME})
            execute_process (
                COMMAND powershell -Command "Get-ChildItem -Path '$ENV{URHO3D_HOME}' -Recurse -Filter 'Urho3D.h' | 
                                                Where-Object { $_.DirectoryName -like \"*include\\Urho3D*\" } | 
                                                Select-Object FullName"
                RESULT_VARIABLE PS_RESULT
                ERROR_VARIABLE FIND_ERRORS
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE ENV_HEADERS
            )
        endif()
    else ()
        execute_process (
            COMMAND find "${${PROJECTNAME}_URHO3D_SEARCH_PATH}" -name "Urho3D.h"
                ! -path "*/.*" ! -path "*/android/*" ! -path "*/bin*/*" ! -path "*/cmake/*"
                ! -path "*/CMake/*" ! -path "*/Docs/*" ! -path "*/gradle/*" ! -path "*/script/*"                
                ! -path "*/Source*/*" ! -path "*/source/*" ! -path "*/SourceAssets/*" ! -path "*/tools/*"
                ! -path "*/website/*" ! -path "*/CMakeFiles/*" ! -path "*/generated/*" ! -path "*/lib/*"
            COMMAND grep "/include/Urho3D/Urho3D.h$"
            ERROR_VARIABLE FIND_ERRORS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE HEADERS
        )
        if(DEFINED ENV{URHO3D_HOME})
            execute_process (
                COMMAND find "$ENV{URHO3D_HOME}" -name "Urho3D.h"
                    ! -path "*/.*" ! -path "*/android/*" ! -path "*/bin*/*" ! -path "*/cmake/*"
                    ! -path "*/CMake/*" ! -path "*/Docs/*" ! -path "*/gradle/*" ! -path "*/script/*"                
                    ! -path "*/Source*/*" ! -path "*/source/*" ! -path "*/SourceAssets/*" ! -path "*/tools/*"
                    ! -path "*/website/*" ! -path "*/CMakeFiles/*" ! -path "*/generated/*" ! -path "*/lib/*"
                COMMAND grep "/include/Urho3D/Urho3D.h$"
                ERROR_VARIABLE FIND_ERRORS
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE ENV_HEADERS
            )
        endif()
    endif ()
    if (FIND_ERRORS)
        message ("urho_find_builds_dirs ERROR: ${FIND_ERRORS}")
        return ()
    endif ()
    if(ENV_HEADERS)
        string(JOIN "\n" HEADERS "${HEADERS}" "${ENV_HEADERS}")
    endif()
    set (NUM_BUILD_DIRS 0)
    if (HEADERS)
        string (REPLACE "\n" ";" HEADERS_LIST "${HEADERS}")
        foreach (HEADER ${HEADERS_LIST})
            if (HEADER MATCHES "Urho3D.h$")
                file (TO_CMAKE_PATH "${HEADER}" DIR)
                string (REPLACE "/include/Urho3D/Urho3D.h" "" DIR ${DIR})
                set (LIB_FILES "")            
                if (EXCLUDE_CROSSBUILD)
                    get_build_target (${DIR} crossdevice)
                    if (crossdevice)
                        message (" ... exclude ${crossdevice} build ${DIR}")
                        continue ()
                    endif ()
                endif ()
                file (GLOB LIB_FILES LIST_DIRECTORIES FALSE 
                        "${DIR}/lib*/*Urho3D.a" "${DIR}/lib*/Urho3D/*Urho3D.a"
                        "${DIR}/lib*/*Urho3D.lib" "${DIR}/lib*/Urho3D/*Urho3D.lib" 
                        "${DIR}/lib*/*Urho3D.so" "${DIR}/lib*/Urho3D/*Urho3D.so" 
                        "${DIR}/lib*/*Urho3D.dll*" "${DIR}/lib*/Urho3D/*Urho3D.dll*" 
                        "${DIR}/lib*/*Urho3D.dylib" "${DIR}/lib*/Urho3D/*Urho3D.dylib")
                if (LIB_FILES)
                    message (" ... library: ${LIB_FILES}")
                    list (APPEND ${PROJECTNAME}_URHO3D_DIRS ${DIR})
                    math (EXPR NUM_BUILD_DIRS "${NUM_BUILD_DIRS} + 1")
                endif()
            endif ()
        endforeach ()
    endif ()
    message ("urho_find_builds_dirs NUM_BUILD_DIRS = ${NUM_BUILD_DIRS}")
endmacro ()

# Generate the tag list for ${PROJECTNAME}_URHO3D_SELECT
function (urho_generate_tags_list taglist)
    unset (${taglist} PARENT_SCOPE)
    foreach (home ${${PROJECTNAME}_URHO3D_DIRS})
        if (home)
            message ("urho_generate_tags_list home = ${home}")
            # Format = shortPathName(category_version_libType_archbuildType)
            unset (root)
            unset (src)
            unset (origin)
            unset (version)

            urho_find_origin ("${home}" root src origin)
            if (NOT origin)
                continue ()
            endif ()
            get_filename_component (shortpath ${root} NAME)
            
            if (origin STREQUAL "source")
                urho_get_revision("${root}" version)
                list (APPEND tags "${shortpath}(SOURCE_${version})")
            elseif (origin STREQUAL "sdk")
                file (STRINGS ${home}/include/Urho3D/Urho3D.h HeaderStrings)
                set (libtype "SHARED")
                # TODO: For the main Urho3D branch, URHO3D_STATIC_DEFINE is only set if MSVC
                foreach (VAR ${HeaderStrings})
                    if (VAR STREQUAL "#define URHO3D_STATIC_DEFINE")
                        set (libtype "STATIC")
                        break ()
                    endif ()
                endforeach ()
                list (APPEND tags "${shortpath}(SDK_${version}_${libtype})")
            elseif (origin STREQUAL "build")
                get_build_target (${home} target)
                get_build_arch (${home} arch)
                get_build_libtype (${home} libtype)
                get_build_buildtype (${home} buildtype)
                if (NOT arch)
                    set (arch ${CMAKE_SYSTEM_PROCESSOR})
                endif ()
                if (NOT target)
                    set (target "${CMAKE_SYSTEM_NAME}")
                endif ()
                list (APPEND tags "${shortpath}(BUILD_${target}_${arch}_${libtype}_${buildtype})") 
            endif () 
        endif ()
    endforeach ()
    if (tags)
        set (${taglist} ${tags} PARENT_SCOPE)
    endif ()
endfunction ()

# Remove all cached variables that don't match the prefix
# Also, remove global variables with the same name as cached variables.
# Always exclude CMAKE_vars
macro (unset_cache_variables_without prefix)
    get_cmake_property(cachedVariables CACHE_VARIABLES)
    string (TOUPPER ${prefix} prefix)
    foreach (variable ${cachedVariables})
        string (TOUPPER ${variable} uppervariable)
        if (NOT "${uppervariable}" MATCHES "^${prefix}" AND
            NOT "${uppervariable}" MATCHES "^CMAKE")
            unset ("${variable}" CACHE)
            unset ("${variable}")
        endif ()
    endforeach ()
endmacro ()

set (${PROJECTNAME}_URHO3D_SEARCH_PATH "" CACHE PATH "Root path for searching Urho3D")
if (NOT ${PROJECTNAME}_URHO3D_SEARCH_PATH)
    if (URHO3D_SEARCH_PATH)
        set (${PROJECTNAME}_URHO3D_SEARCH_PATH "${URHO3D_SEARCH_PATH}" CACHE PATH "Root path for searching Urho3D" FORCE)
        unset (URHO3D_SEARCH_PATH)
    else ()
        set (${PROJECTNAME}_URHO3D_SEARCH_PATH "${CMAKE_SOURCE_DIR}" CACHE PATH "Root path for searching Urho3D" FORCE)
    endif ()
endif ()

# Conditions to launch the search of Urho3D directories
if ((NOT URHO3D_HOME AND NOT ${PROJECTNAME}_URHO3D_DIRS) OR FORCEDISCOVER OR
    (DEFINED ${PROJECTNAME}_URHO3D_SEARCH_PATH_LAST AND NOT "${${PROJECTNAME}_URHO3D_SEARCH_PATH_LAST}" STREQUAL "${${PROJECTNAME}_URHO3D_SEARCH_PATH}"))
    set (${PROJECTNAME}_URHO3D_SEARCH_PATH_LAST ${${PROJECTNAME}_URHO3D_SEARCH_PATH} CACHE INTERNAL STRING)        
    set (SEARCH_URHO3D_ENABLE TRUE)
    unset (FORCEDISCOVER CACHE)    
else ()
    set (SEARCH_URHO3D_ENABLE FALSE)
endif ()

if (SEARCH_URHO3D_ENABLE)
    message (" .. Searching for Urho3D directories in path = ${${PROJECTNAME}_URHO3D_SEARCH_PATH} (this may take some time)")

    unset (${PROJECTNAME}_URHO3D_DIRS)
    unset (${PROJECTNAME}_URHO3D_DIRS CACHE)
    unset (${PROJECTNAME}_URHO3D_TAGS)
    unset (${PROJECTNAME}_URHO3D_TAGS CACHE)
    urho_find_sources_dirs ()
    urho_find_builds_dirs ()

    if (${PROJECTNAME}_URHO3D_DIRS)
        list (SORT ${PROJECTNAME}_URHO3D_DIRS)
        urho_generate_tags_list (taglist)
    endif ()

    set (num_tags 0)
    if (taglist)
        list (LENGTH taglist num_tags)
    endif ()
    message (" ... Found ${num_tags} Urho3D directories!")

    set (${PROJECTNAME}_URHO3D_DIRS ${${PROJECTNAME}_URHO3D_DIRS} CACHE INTERNAL "All available Urho3D directories")
    set (${PROJECTNAME}_URHO3D_TAGS ${taglist} CACHE INTERNAL "All available Urho3D tags")
    set (${PROJECTNAME}_URHO3D_SELECT "" CACHE STRING "Urho3D source/build tree selection" FORCE)
    set_property (CACHE ${PROJECTNAME}_URHO3D_SELECT PROPERTY STRINGS ${${PROJECTNAME}_URHO3D_TAGS})
endif ()

# Update the Urho3D directory selection
if (NOT "${${PROJECTNAME}_URHO3D_SELECT_LAST}" STREQUAL "${${PROJECTNAME}_URHO3D_SELECT}")
    set (${PROJECTNAME}_URHO3D_SELECT_LAST ${${PROJECTNAME}_URHO3D_SELECT} CACHE INTERNAL STRING)
    unset_cache_variables_without ("${PROJECTNAME}")

    # TODO : to test
    if (CMAKE_CROSSCOMPILING)
        # include the toolchain to be sure to have well-defined cross-compile variables
        include (${CMAKE_TOOLCHAIN_FILE})
    endif ()
    
    unset (URHO3D_ROOT_DIR)
    unset (URHO3D_SOURCE_DIR)
    unset (URHO3D_HOME)
    unset (URHO3D_HOME CACHE)
endif ()

# Set Urho3D home directory based on the selected folder
if (${PROJECTNAME}_URHO3D_SELECT AND NOT URHO3D_HOME_LAST)
    set (URHO3D_HOME_LAST ${URHO3D_HOME})
    list (FIND ${PROJECTNAME}_URHO3D_TAGS "${${PROJECTNAME}_URHO3D_SELECT}" index)
    if (NOT index EQUAL -1)
        list (GET ${PROJECTNAME}_URHO3D_DIRS ${index} home)
        if (EXISTS ${home})
            set (URHO3D_HOME ${home} CACHE PATH "Urho3D source, build tree, or SDK directory" FORCE)
            set (${PROJECTNAME}_URHO3D_DIR_SELECT ${home} CACHE INTERNAL STRING)
        endif ()
    endif ()
endif ()

# Align the selection with URHO3D_HOME if it exists
if (URHO3D_HOME AND NOT "${${PROJECTNAME}_URHO3D_DIR_SELECT}" STREQUAL "${URHO3D_HOME}")
    list (FIND ${PROJECTNAME}_URHO3D_DIRS "${URHO3D_HOME}" index)
    if (NOT index EQUAL -1)
        set (${PROJECTNAME}_URHO3D_DIR_SELECT ${URHO3D_HOME} CACHE INTERNAL STRING)
        list (GET ${PROJECTNAME}_URHO3D_TAGS ${index} tag)
        set (${PROJECTNAME}_URHO3D_SELECT "${tag}" CACHE STRING "Urho3D source/build tree selection" FORCE)
    endif ()
endif ()
