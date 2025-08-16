@echo off
setlocal

set CHANNEL=%1
if "%CHANNEL%"=="" set CHANNEL=debug

set BUILD_DIR=build\%CHANNEL%
set CMAKE_TYPE=

if /I "%CHANNEL%"=="debug" set CMAKE_TYPE=Debug
if /I "%CHANNEL%"=="release" set CMAKE_TYPE=Release
if /I "%CHANNEL%"=="relwithdebinfo" set CMAKE_TYPE=RelWithDebInfo
if /I "%CHANNEL%"=="minsizerel" set CMAKE_TYPE=MinSizeRel

if "%CMAKE_TYPE%"=="" (
    echo Unknown build channel: %CHANNEL%
    echo Usage: compile.bat [debug|release|relwithdebinfo|minsizerel]
    exit /b 1
)

echo 🔧 Building in %BUILD_DIR% (type: %CMAKE_TYPE%)...
cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%CMAKE_TYPE%
cmake --build %BUILD_DIR%
echo ✅ Build complete.

endlocal
