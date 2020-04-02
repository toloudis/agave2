For windows:
Make sure you are in an environment where vsvarsall has been run, e.g. a "VS2019 x64 Native Tools Command Prompt"

```
mkdir build
cd build
# (vs 2019)
cmake -DCMAKE_TOOLCHAIN_FILE=D:\vcpkg\scripts\buildsystems\vcpkg.cmake -G "Visual Studio 16 2019" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows ..
cmake --build .
```
