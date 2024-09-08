@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl -Zi ..\code\chip8.cpp /nologo

popd
