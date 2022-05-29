@echo off
mkdir bin
pushd src
cl -Zi ^
  /O2 ^
    main.cpp ^
  opengl32.lib ^
  ..\libs\lib\pthreadVC2.lib  ^
  ..\libs\lib\sfml-system.lib ^
  ..\libs\lib\sfml-window.lib ^
  ..\libs\lib\sfml-graphics.lib ^
   -o ..\bin\main.exe ^
   /I ..\libs\include
popd

pushd bin
main
popd
