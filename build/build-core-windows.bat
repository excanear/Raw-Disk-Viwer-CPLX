@echo off
:: ================================================================
:: build-core-windows.bat
:: Compiles the RDVC C core library into rdvc_core.dll (Windows x64)
::
:: Requirements:
::   - MinGW-w64 / GCC in PATH  (mingw32-make, gcc)
::   - Run from the project root:
::       Raw Disk Viwer CPLX\
:: ================================================================

setlocal enabledelayedexpansion

echo [RDVC] Building C Core (Windows x64)...

set OUTDIR=build\output
set SRCDIR=core\src
set INCDIR=core\include
set OUTLIB=%OUTDIR%\rdvc_core.dll

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

:: Collect all .c source files
set SOURCES=
for %%f in (%SRCDIR%\*.c) do (
    set SOURCES=!SOURCES! %%f
)

gcc -shared -O2 -m64 ^
    -DRDVC_BUILDING_DLL ^
    -DRDVC_WINDOWS ^
    -I%INCDIR% ^
    %SOURCES% ^
    -Wl,--out-implib,%OUTDIR%\\librdvc_core.dll.a ^
    -lkernel32 -ladvapi32 ^
    -o %OUTLIB%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Core build failed.
    exit /b 1
)

echo [OK] Output: %OUTLIB%
endlocal
