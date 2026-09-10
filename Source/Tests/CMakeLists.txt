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

if (NOT URHO3D_UNIT_TESTS)
    return ()
endif ()

project (Urho3D-Tests)

find_package (Urho3D REQUIRED)
include_directories (${URHO3D_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Catch2)

set (TARGET_NAME Urho3D-UnitTests)

define_source_files (RECURSE GROUP)

add_definitions (-DU3D_TEST_RESOURCE_PREFIX="${CMAKE_SOURCE_DIR}/bin")
add_definitions (-DU3D_TEST_SCRATCH_DIR="${CMAKE_BINARY_DIR}/test-scratch")

setup_executable (PRIVATE)

foreach (TAG Math Container Core IO Engine Input Network Scene Resource UI Physics Physics2D Urho2D Graphics Audio Navigation IK Base)
    add_test (NAME ${TAG} COMMAND ${TARGET_NAME} [${TAG}] --order decl --warn NoAssertions)
    set_tests_properties (${TAG} PROPERTIES TIMEOUT 600)
endforeach ()
