@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"
:restart

:: --- Unpack Arguments -------------------------------------------------------
for %%a in (%*) do set "%%~a=1"

if not "%release%"=="1" set debug=1

if "%debug%"=="1"   set release=0
if "%release%"=="1" set debug=0

:: --- Compile/Link Line Definitions ------------------------------------------
if "%debug%"=="1"   set outname=debug
if "%release%"=="1" set outname=drt1_logi_g29_server

set cl_common=     /EHsc /nologo /MD /Fe:%outname% /FC /Z7 /Zc:preprocessor User32.lib Advapi32.lib shell32.lib Ws2_32.lib .\..\lib\hidapi.lib .\..\lib\libiniparser.lib
set cl_debug=      call cl /Od /Ob1 /DBUILD_DEBUG=1 %cl_common%
set cl_release=    call cl /O2 /DBUILD_DEBUG=0 %cl_common%

set compile_debug=%cl_debug%
set compile_release=%cl_release%

if "%debug%"=="1"     set compile=%compile_debug%
if "%release%"=="1"   set compile=%compile_release%

:: --- Prep Directories -------------------------------------------------------
set build_dir=.build

if not exist %build_dir% mkdir %build_dir%

:: --- Build --------------------------------------
pushd %build_dir%

if "%debug%"=="1" echo [compile debug]
if "%release%"=="1" echo [compile release]

del /Q *.*

%compile% ..\server.c || exit /b 1

popd

if "%autorun%"=="1" call .\%build_dir%\%outname%.exe