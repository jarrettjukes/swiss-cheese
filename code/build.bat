@echo off

if not exist ..\build\ mkdir ..\build\
pushd ..\build\
del *.pdb
echo Debug build
cl ..\code\main.cpp -Z7 -Od -FC -MTd -WX -W4 -wd4996 -wd4189 -wd4100 -nologo -link user32.lib -incremental:no -opt:ref
REM cl ..\code\main.cpp -Z7 -O2 -FC -MTd -WX -W4 -wd4996 -wd4189 -nologo -fp:fast -link user32.lib -incremental:no -opt:ref
popd