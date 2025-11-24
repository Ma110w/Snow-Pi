@echo off
REM Quick Windows compile script for Snow-Pi Dashboard
REM Author: /x64/dumped

echo Compiling Snow-Pi Dashboard for Windows...

set SDL_DIR=SDL-release-3.2.26\SDL-release-3.2.26
set BUILD_DIR=SDL-release-3.2.26\build\Release

REM Check if SDL3 is built
if not exist "%BUILD_DIR%\SDL3.dll" (
    echo Error: SDL3.dll not found in %BUILD_DIR%
    echo Please run build-windows.bat first to build SDL3
    exit /b 1
)

REM Compile
gcc -Wall -Wextra -O2 -std=c11 ^
    -I%SDL_DIR%\include ^
    -o snow-pi-dash.exe ^
    main.c ^
    -L%BUILD_DIR% ^
    -lSDL3 -lSDL3_ttf -lm

if errorlevel 1 (
    echo Compilation failed!
    exit /b 1
)

REM Copy SDL3.dll and SDL3_ttf.dll to current directory for easy running
copy "%BUILD_DIR%\SDL3.dll" . >nul 2>&1
copy "%BUILD_DIR%\SDL3_ttf.dll" . >nul 2>&1

echo.
echo ==========================================
echo Build successful!
echo ==========================================
echo.
echo Run with: snow-pi-dash.exe
echo Press ESC or Q to quit
echo.

