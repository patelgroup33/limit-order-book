@echo off
REM build.bat - configure and build the whole project (library, tests, simulator).
REM
REM Usage:
REM   scripts\build.bat [BUILD_TYPE]     BUILD_TYPE defaults to Release
setlocal
set "ROOT=%~dp0.."
set "BUILD_DIR=%ROOT%\build"
set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"

echo ==^> Configuring (%BUILD_TYPE%) in %BUILD_DIR%
cmake -S "%ROOT%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 exit /b 1

echo ==^> Building
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 exit /b 1

echo ==^> Done. Simulator: %BUILD_DIR%\sim\%BUILD_TYPE%\lob_sim.exe
endlocal
