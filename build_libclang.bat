@echo off

echo This will take awhile so go eat something, do something. Live laugh love

pushd thirdparty\llvm-project
 mkdir build-release
 pushd build_release
  
  cmake -GNinja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release ..\llvm
  cmake --build .

 popd
popd

echo Done
