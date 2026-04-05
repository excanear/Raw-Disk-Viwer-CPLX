@echo off
REM Launcher for RAW DISK VIEWER CPLX (Windows)
setlocal
set "ROOT=%~dp0"
set "LIB=%ROOT%build\output"
set "MSYS=%SystemDrive%\msys64\mingw64\bin"
if exist "%MSYS%" (
  set "PATH=%LIB%;%MSYS%;%PATH%"
) else (
  set "PATH=%LIB%;%PATH%"
)
java --enable-preview -Djava.library.path="%LIB%" -cp "%ROOT%gui\target\raw-disk-viewer-cplx-1.0.0-shaded.jar" com.rdvc.Launcher
if %ERRORLEVEL% neq 0 pause
endlocal
