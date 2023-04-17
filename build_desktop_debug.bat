@echo off

echo If this fails make sure you cloned with submodules (or go download the submodules) then run build_libclang.bat and build_raylib.bat

cl^
 thirdparty\llvm-project\build-release\lib\libclang.lib^
 /Ithirdparty\llvm-project\clang\include^
 thirdparty\raylib\build-release\raylib\raylib.lib^
 /Ithirdparty\raylib\build-release\raylib\include^
 /Ithirdparty^
 /MT gdi32.lib user32.lib shell32.lib msvcrt.lib vcruntime.lib Winmm.lib^
 /Fe"introspect_debug"^
 /W3 /Zi /WX main.c 
