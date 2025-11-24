@echo off
REM Snow-Pi Dashboard Windows Build Script
REM Author: /x64/dumped

echo ==========================================
echo Building SDL3 for Snow-Pi Dashboard (Windows)
echo ==========================================

set SDL_SOURCE=SDL-release-3.2.26\SDL-release-3.2.26
set BUILD_DIR=SDL-release-3.2.26\build

if not exist "%SDL_SOURCE%" (
    echo Error: SDL3 source not found at %SDL_SOURCE%
    echo Make sure you've extracted SDL-release-3.2.26.zip
    exit /b 1
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo Configuring SDL3...
cmake ..\SDL-release-3.2.26 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DSDL_STATIC=OFF ^
    -DSDL_SHARED=ON ^
    -DSDL_TEST_LIBRARY=OFF

if errorlevel 1 (
    echo CMake configuration failed!
    cd ..\..
    exit /b 1
)

echo Building SDL3 (this may take a few minutes)...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo Build failed!
    cd ..\..
    exit /b 1
)

cd ..\..

echo.
echo ==========================================
echo SDL3 build complete!
echo ==========================================
echo.
echo Now compile main.c with your C compiler
echo.

