@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "MAKE_EXE="
for /f "delims=" %%I in ('where make 2^>nul') do if not defined MAKE_EXE set "MAKE_EXE=%%I"
if not defined MAKE_EXE (
    for /f "delims=" %%I in ('where mingw32-make 2^>nul') do if not defined MAKE_EXE set "MAKE_EXE=%%I"
)

set "GXX_EXE="
for /f "delims=" %%I in ('where g++ 2^>nul') do if not defined GXX_EXE set "GXX_EXE=%%I"

set "TOOLCHAIN_BIN="
if defined MAKE_EXE for %%I in ("%MAKE_EXE%") do set "TOOLCHAIN_BIN=%%~dpI"
if not defined TOOLCHAIN_BIN if defined GXX_EXE for %%I in ("%GXX_EXE%") do set "TOOLCHAIN_BIN=%%~dpI"
if defined TOOLCHAIN_BIN set "PATH=%TOOLCHAIN_BIN%;%PATH%"

set "TARGET="
if exist "star_abyss_salvager.exe" set "TARGET=star_abyss_salvager.exe"
if not defined TARGET if exist "star_abyss_salvager" set "TARGET=star_abyss_salvager"

if not defined TARGET (
    if not defined MAKE_EXE (
        echo Could not find make or mingw32-make in PATH.
        pause
        exit /b 1
    )

    call "%MAKE_EXE%"
    if errorlevel 1 (
        echo Build failed.
        pause
        exit /b %errorlevel%
    )

    if exist "star_abyss_salvager.exe" set "TARGET=star_abyss_salvager.exe"
    if not defined TARGET if exist "star_abyss_salvager" set "TARGET=star_abyss_salvager"
)

if not defined TARGET (
    echo Build finished, but the game executable was not found.
    pause
    exit /b 1
)

".\%TARGET%"
if errorlevel 1 (
    echo Game exited with error code %errorlevel%.
    pause
    exit /b %errorlevel%
)
