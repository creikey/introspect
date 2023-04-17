@echo off

echo If this fails make sure you cloned with submodules (or go download the submodules) then run build_libclang.bat and build_raylib.bat


set ARGS=""

if "%1" == "release" (
 set ARGS=/Fe"introspect_release" /O2
) else (
 set ARGS=/Fe"introspect_debug" /Zi
)

cl^
 thirdparty\llvm-project\build-release-static\lib\*.lib^
 /Ithirdparty\llvm-project\clang\include^
 thirdparty\raylib\build-release\raylib\raylib.lib^
 /Ithirdparty\raylib\build-release\raylib\include^
 /Ithirdparty^
 /MT gdi32.lib user32.lib shell32.lib msvcrt.lib vcruntime.lib Winmm.lib Version.lib^
 /W3 /WX %ARGS% main.c 


if "%1" == "release" (
 echo Built release build
) else (
 echo Built debug build. For release do `build_desktop.bat release`
)
