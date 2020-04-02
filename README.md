## Windows:

Make sure you are in an environment where vsvarsall has been run, e.g. a "VS2019 x64 Native Tools Command Prompt"

```
mkdir build
cd build
# (vs 2019)
cmake -DCMAKE_TOOLCHAIN_FILE=D:\vcpkg\scripts\buildsystems\vcpkg.cmake -G "Visual Studio 16 2019" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows ..

cmake --build . --config Release --target agave2app
cmake --build . --config Debug --target agave2app

TODO
cmake --build . --config Debug --target install

```

## MacOS

https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html
Use the install.py script to ensure the sdk is in /usr/local to make pathing easier
