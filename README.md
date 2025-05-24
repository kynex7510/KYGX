# CTRGX

Low level abstraction of the GX graphics system for the 3DS.

## Build

Download a [prebuilt](https://github.com/kynex7510/CTRGX/releases) version, use as a CMake dependency, or build manually:

```sh
cmake -B Build -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DDKP_NO_BUILTIN_CMAKE_CONFIGS=1 -DCMAKE_BUILD_TYPE=Release
cmake --build Build --config Release
cmake --install Build --prefix Build/Release
```