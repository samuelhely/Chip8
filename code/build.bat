@echo off

set LINKER_FLAGS=shell32.lib user32.lib raylib.lib gdi32.lib opengl32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl -WX -Wall -W4 -wd4820 -wd4189 -wd5045 -wd5219 -wd4996 -wd4365 -wd4456 -wd4100 -wd4061 -wd4267 -wd4505 -Zi -FC -nologo -I..\code\raylib\include ..\code\chip8.cpp %LINKER_FLAGS% /link -libpath:..\code\raylib\lib -NODEFAULTLIB:libcmt
REM cl -Zi -FC -nologo -Ic:\raylib\raylib\src ..\code\chip8.cpp %LINKER_FLAGS% /link -libpath:c:\raylib\raylib\src\libraylib.a -NODEFAULTLIB:libcmt
REM cl -Zi ..\code\chip8.cpp nologo

popd
