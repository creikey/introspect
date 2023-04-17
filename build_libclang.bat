@echo off

echo This will take awhile so go eat something, do something. Live laugh love

pushd thirdparty\llvm-project
 mkdir build-release-static
 pushd build-release-static
  
 cmake -GNinja -DLIBCLANG_BUILD_STATIC=ON -DLLVM_ENABLE_PIC=FALSE -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release ..\llvm
 cmake --build .

 popd
 popd

 echo Done
