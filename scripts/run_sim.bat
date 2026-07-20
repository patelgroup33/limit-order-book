@echo off
REM run_sim.bat - build the simulator, generate a replay, and open the visualizer.
REM
REM Usage:
REM   scripts\run_sim.bat [STEPS] [PORT]
REM     STEPS  number of order-flow steps (default 1200)
REM     PORT   visualizer port            (default 8137)
setlocal
set "ROOT=%~dp0.."
set "BUILD_DIR=%ROOT%\build"
set "STEPS=%~1"
if "%STEPS%"=="" set "STEPS=1200"
set "PORT=%~2"
if "%PORT%"=="" set "PORT=8137"

echo ==^> Building simulator
REM Tests off so the simulator build needs no GoogleTest fetch (just "see it run").
cmake -S "%ROOT%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release -DLOB_BUILD_TESTS=OFF
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%" --target lob_sim --config Release --parallel
if errorlevel 1 exit /b 1

REM Multi-config generators (Visual Studio) put the exe under a config subfolder;
REM single-config generators put it directly under sim\.
set "SIM_EXE=%BUILD_DIR%\sim\Release\lob_sim.exe"
if not exist "%SIM_EXE%" set "SIM_EXE=%BUILD_DIR%\sim\lob_sim.exe"

echo ==^> Generating replay data (%STEPS% steps) -^> web\simulation.json
"%SIM_EXE%" %STEPS% "%ROOT%\web\simulation.json"
if errorlevel 1 exit /b 1

echo ==^> Serving visualizer at http://localhost:%PORT%  (press Ctrl+C to stop)
start "" "http://localhost:%PORT%"
cd /d "%ROOT%\web"
python -m http.server %PORT%
endlocal
