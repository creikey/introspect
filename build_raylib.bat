@echo off

echo Much shorter than raylib

pushd thirdparty\raylib

 del CMakeCache.txt

 mkdir build-release
 pushd build-release

  cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF ..
  cmake --build .

 popd

popd
