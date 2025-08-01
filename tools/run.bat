@echo off
setlocal

set CHANNEL=%1
if "%CHANNEL%"=="" set CHANNEL=debug

set TARGET=%2
if "%TARGET%"=="" set TARGET=tests

set BUILD_DIR=build\%CHANNEL%

if /I "%TARGET%"=="tests" (
    echo Running tests for channel: %CHANNEL%
    ctest --test-dir %BUILD_DIR% --output-on-failure
    goto :end
)

if /I "%TARGET%"=="benchmarks" (
    echo Running benchmarks for channel: %CHANNEL%
    %BUILD_DIR%\oxidize_benchmarks.exe
    goto :end
)

echo Unknown target: %TARGET%
echo Usage: run.bat [channel] [tests|benchmarks]

:end
endlocal
