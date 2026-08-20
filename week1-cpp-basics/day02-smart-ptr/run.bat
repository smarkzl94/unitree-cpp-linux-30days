@echo off
cd /d "%~dp0..\..\.."
set "PATH=%CD%\tools\w64devkit\bin;%PATH%"
cmake --preset default
if errorlevel 1 goto :fail
cmake --build --preset default --target day02
if errorlevel 1 goto :fail
echo.
build\day02.exe
echo.
pause
exit /b 0
:fail
echo.
echo cmake build failed
pause
exit /b 1
