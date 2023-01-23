#
# Copyright (c) 2008-2023 the Urho3D project.
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

# Define target name
set (TARGET_NAME fmt)

# Define source files. We do not want src/fmt.cc, as that is only for the c++ module.
file (GLOB CPP_FILES src/format.cc src/os.cc)
file (GLOB H_FILES include/fmt/*.h)

# Set source files and add include so the library can use "fmt/___.h" as the #include line.
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})
set (INCLUDE_DIRS include)

# Do not setup library as we are using it header-only (`#define FMT_HEADER_ONLY` has been added to format.h)
#setup_library ()

# Install headers for building and using the Urho3D library (install dependency for Urho3D/Core/StringUtils.h)
install_header_files (DIRECTORY include/fmt/ DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/fmt FILES_MATCHING PATTERN *.h)  # Note: the trailing slash is significant
