@echo off
:: ================================================================
:: build-jni-windows.bat
:: Compiles the JNI bridge into rdvc_jni.dll (Windows x64)
::
:: Requirements:
::   - JAVA_HOME must point to JDK 21+
::   - rdvc_core.dll must already be in build\output\
::   - MinGW-w64 / GCC in PATH
::   - Run from the project root
:: ================================================================

setlocal enabledelayedexpansion

echo [RDVC] Building JNI Bridge (Windows x64)...

if not defined JAVA_HOME (
    echo [ERROR] JAVA_HOME is not set. Point it to your JDK 21 installation.
    exit /b 1
)

set OUTDIR=build\output
set JNISRC=jni\src
set COREINC=core\include
set JAVA_INC=%JAVA_HOME%\include
set JAVAWIN_INC=%JAVA_HOME%\include\win32
set OUTLIB=%OUTDIR%\rdvc_jni.dll
set CORELIB=%OUTDIR%\rdvc_core.dll

if not exist "%CORELIB%" (
    echo [ERROR] rdvc_core.dll not found at %CORELIB%.
    echo         Run build-core-windows.bat first.
    exit /b 1
)

:: Generate JNI headers from compiled Java classes (if javac was run)
:: javah is deprecated since JDK 10 — headers are included manually.

:: Collect JNI sources
set SOURCES=
for %%f in (%JNISRC%\*.c) do (
    set SOURCES=!SOURCES! %%f
)

gcc -shared -O2 -m64 ^
    -DRDVC_WINDOWS ^
    -I"%COREINC%" ^
    -I"%JNISRC%\..\include" ^
    -I"%JAVA_INC%" ^
    -I"%JAVAWIN_INC%" ^
    %SOURCES% ^
    -Wl,--enable-auto-import ^
    -L%OUTDIR% -lrdvc_core ^
    -lkernel32 ^
    -o %OUTLIB%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] JNI bridge build failed.
    exit /b 1
)

echo [OK] Output: %OUTLIB%
echo.
echo IMPORTANT: Copy rdvc_core.dll and rdvc_jni.dll next to your JAR,
echo            or set -Djava.library.path=build\output when launching.
endlocal
