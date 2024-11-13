#!/usr/bin/env bash
#
# Copyright (c) 2008-2022 the Urho3D project.
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

if [ -z "$EMSDK" ]; then
    if [ -z "$EMSCRIPTEN_ROOT_PATH" ]; then
        echo " "
        echo "Error: The EMSCRIPTEN_ROOT_PATH variable is not defined."
        echo " "
        echo "if you don't have Emscripten installed, follow the instructions at https://emscripten.org/docs/getting_started/downloads.html"
        echo " "
        echo "you don't need to use emsdk_env.sh or add it to your bash profile."
        echo "just need to set EMSCRIPTEN_ROOT_PATH (the path to emscripten sdk)."
        echo " "
        exit 1
    fi

    source $EMSCRIPTEN_ROOT_PATH/emsdk_env.sh
fi

emcmake $(dirname $0)/cmake_generic.sh "$@" -D WEB=1

# vi: set ts=4 sw=4 expandtab:
