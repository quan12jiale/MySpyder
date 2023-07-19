@echo off
cd %~dp0
set tools=%~dp0
if not exist "%tools%\..\..\build" md "%tools%\..\..\build"

set pathToSource=%~dp0

::平台
::set platform=Win32
set platform=x64

set config=RelWithDebInfo

set pathToBuild=%~dp0\..\..\build\Sln\%platform%\%config%

::生成器
set generator="Visual Studio 14 2015"
::set generator="Visual Studio 17 2022"


cmake -S %pathToSource% -B %pathToBuild% -G%generator% -A %platform% -D CMAKE_CONFIGURATION_TYPES=%config%

pause