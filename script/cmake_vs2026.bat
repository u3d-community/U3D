::
:: Copyright (c) 2008-2022 the Urho3D project.
:: Copyright (c) 2022-2026 the U3D project.
::
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to deal
:: in the Software without restriction, including without limitation the rights
:: to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
:: copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
::
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
::
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
:: OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
:: THE SOFTWARE.
::


:: Configuring for Visual Studio 2026 requires minimum CMake version of 4.2. This 
:: script ensures that the installed CMake version meets this requirement pending 
:: when the minimum required CMake version of the overall project is bumped to 4.2 or higher.

@echo off
setlocal enabledelayedexpansion

:: The minimum required version of CMake is 4.2
set "REQ_MAJOR=4"
set "REQ_MINOR=2"

:: Get CMake version string (e.g., "cmake version 4.2.0")
for /f "tokens=3" %%a in ('cmake --version ^| findstr /i "version"') do (
    set "FULL_VERSION=%%a"
)

:: Split the version into Major.Minor.Patch
for /f "tokens=1,2,3 delims=." %%a in ("%FULL_VERSION%") do (
    set "CUR_MAJOR=%%a"
    set "CUR_MINOR=%%b"
)

:: Logic: Check Major first, then Minor if Major is equal
if %CUR_MAJOR% gtr %REQ_MAJOR% goto :success
if %CUR_MAJOR% equ %REQ_MAJOR% (
    if %CUR_MINOR% geq %REQ_MINOR% goto :success
)

:failure
echo ---------------------------------------------------------
echo ERROR: CMake %CUR_MAJOR%.%CUR_MINOR% found but %REQ_MAJOR%.%REQ_MINOR% or higher is required to configure U3D for Visual Studio 2026.
echo Please update CMake and try again.
echo ---------------------------------------------------------
exit /b 1

:success

@"%~dp0cmake_generic.bat" %* -VS=18
