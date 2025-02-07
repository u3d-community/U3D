#
# Copyright (c) 2008-2022 the Urho3D project.
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

if (CMAKE_PROJECT_NAME STREQUAL Urho3D OR URHO3D_AS_SUBMODULE)
    # Set project name
    project (Urho3D-Clang-Tools)

    # All Clang-tools must be natively built and output to bin/tool/clang subdir to differentiate them from the rest
    set_output_directories (${URHO3D_BUILD_DIR}/bin/tool/clang RUNTIME PDB)
else ()
    # Set CMake minimum version
    cmake_minimum_required (VERSION 3.10.2)

    # URHO3D_CLANG_TOOLS renamed URHO3D_CLANGTOOL
    # set default tool
    if (NOT URHO3D_CLANGTOOL)
        set (URHO3D_CLANGTOOL AutoBinder)
        set (DEST_RUNTIME_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin/tool/clang)
        set (DEST_INCLUDE_DIR include/Urho3D)
    endif ()

    # Set project name
    project (Urho3D-ExternalProject-${URHO3D_CLANGTOOL})
    
    # Set CMake modules search path
    set (CMAKE_MODULE_PATH ${URHO3D_CMAKE_MODULE})

    # Include UrhoCommon.cmake module after setting project name
    include (UrhoCommon)

    # Setup SDK-like include dir in the build tree for building the Clang-tools
    file (MAKE_DIRECTORY ${URHO3D_BUILD_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)
endif ()

# LLVM/Clang is assumed to be installed in a system-wide location when not explicitly defined using env-var
if (NOT DEFINED ENV{LLVM_CLANG_ROOT})
    # TODO : Find LLVM/Clang
endif ()

if (NOT LLVM_CLANG_ROOT AND DEFINED ENV{LLVM_CLANG_ROOT})
    set (LLVM_CLANG_ROOT $ENV{LLVM_CLANG_ROOT})
endif ()

if (NOT LLVM_CONFIG AND LLVM_CLANG_ROOT)
    set (LLVM_CONFIG "${LLVM_CLANG_ROOT}/bin/llvm-config")
endif ()

# Define cxx flags
execute_process (COMMAND ${LLVM_CONFIG} --cppflags OUTPUT_VARIABLE LLVM_DEFINES OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
set (CMAKE_CXX_FLAGS "${LLVM_DEFINES}")
# No exception and no RTTI
if (MSVC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c- /GR-")
else ()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif ()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set (CMAKE_CXX_STANDARD 17)

# Define include dirs
set (INCLUDE_DIRS ${LLVM_CLANG_ROOT}/include ${URHO3D_BUILD_DIR}/${DEST_INCLUDE_DIR}/ThirdParty)

# Define libraries to link
execute_process (COMMAND ${LLVM_CONFIG} --libfiles OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process (COMMAND ${LLVM_CONFIG} --system-libs OUTPUT_VARIABLE LLVM_SYSLIBS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
string (REGEX REPLACE " *-l" ";" LLVM_LIBS "${LLVM_LIBS}")   # Stringify against empty output variable
string (REGEX REPLACE " *-l" ";" LLVM_SYSLIBS "${LLVM_SYSLIBS}")
set (CLANG_LIBS clangTooling clangFormat clangToolingInclusions clangFrontend clangDriver clangParse clangSerialization clangSema clangEdit 
                clangAnalysis clangToolingCore clangRewrite clangASTMatchers clangAST clangLex clangSupport clangBasic)
set (LIBS ${CLANG_LIBS} ${LLVM_LIBS} ${LLVM_SYSLIBS})
link_directories (${LLVM_CLANG_ROOT}/lib)

# Clang-tools can be built in two ways: on the fly in normal build one at a time or build all of them in a special Clang-tools build tree (for development)
if (CMAKE_PROJECT_NAME MATCHES ExternalProject)
    # Externally build the Clang-tool for actual use in a normal build
    add_subdirectory (${URHO3D_CLANGTOOL})
else ()
    # Define source files for the tools
    get_target_property (SOURCES Urho3D SOURCES)
    string (REGEX REPLACE "[^;]+\\.h" "" SOURCES "${SOURCES}")   # Stringify to preserve the semicolons
    string (REGEX REPLACE "[^;]+generated[^;]+\\.cpp" "" SOURCES "${SOURCES}")
    file (GLOB BINDING_SOURCES RELATIVE ${URHO3D_SOURCE_DIR}/Urho3D ${URHO3D_SOURCE_DIR}/Urho3D/Script/*API.cpp)
    string (REGEX REPLACE "[^;]+API\\.cpp" "" ANNOTATED_SOURCES "${SOURCES}")

    # List of tools
    add_subdirectory (Annotator)
    add_subdirectory (AutoBinder)

    # List of targets
    execute_process (COMMAND ${LLVM_CONFIG} --bindir OUTPUT_VARIABLE LLVM_BINDIR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if (EXISTS ${LLVM_BINDIR}/clang-query)  # This tool is from clang-tools-extra repository which user may have not installed
        add_custom_target (ast-query
            COMMAND ${CMAKE_COMMAND} -E echo "Building AST for query, please be patient..."
            COMMAND ${LLVM_BINDIR}/clang-query -p ${CMAKE_CURRENT_BINARY_DIR} $$option ${SOURCES}
            WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
            COMMENT "Executing clang-query on Urho3D library source files")
    endif ()
    if (EXISTS ${LLVM_BINDIR}/clang-check)
        add_custom_target (ast
            COMMAND ${CMAKE_COMMAND} -E echo "Usage: option=-help make ast"
                COMMAND ${LLVM_BINDIR}/clang-check -p ${CMAKE_CURRENT_BINARY_DIR} $$option ${SOURCES}
                WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
            COMMENT "Executing clang-check on Urho3D library source files")
        add_custom_target (binding-ast
            COMMAND ${CMAKE_COMMAND} -E echo "Usage: option=-help make binding-ast"
                COMMAND ${LLVM_BINDIR}/clang-check -p ${CMAKE_CURRENT_BINARY_DIR} $$option ${BINDING_SOURCES}
                WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
            COMMENT "Executing clang-check on (existing) AngelScript API bindings source files")
    endif ()
    add_custom_target (annotate
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/tool/clang/Annotator -p ${CMAKE_CURRENT_BINARY_DIR} ${SOURCES}
        DEPENDS Annotator
        WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
        COMMENT "Annotating Urho3D library source files")
    add_custom_target (autobinder
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_CURRENT_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated -s AngelScript -s LuaScript -s JavaScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
        COMMENT "Auto-binding for all script subsystems")
    add_custom_target (autobinder-angelscript
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_CURRENT_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated -s AngelScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
        COMMENT "Auto-binding for AngelScript")
    add_custom_target (autobinder-luascript
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_CURRENT_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated -s LuaScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
        COMMENT "Auto-binding for LuaScript")
    add_custom_target (autobinder-javascript
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/tool/clang/AutoBinder -p ${CMAKE_CURRENT_BINARY_DIR} -t ${CMAKE_CURRENT_SOURCE_DIR}/AutoBinder/Templates -o ${CMAKE_CURRENT_BINARY_DIR}/Source/Urho3D/generated -s JavaScript ${ANNOTATED_SOURCES}
        DEPENDS AutoBinder
        WORKING_DIRECTORY ${URHO3D_SOURCE_DIR}/Urho3D
        COMMENT "Auto-binding for JavaScript")
endif ()
