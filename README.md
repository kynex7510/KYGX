# KYGX

Low level abstraction of the GX graphics system for the 3DS.

## Setup

Download a [prebuilt](https://github.com/kynex7510/KYGX/releases) version, use as a CMake dependency, or build manually.

### libctru backend (HOS)

```sh
cmake -B BuildHOS -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release -DKYGX_ENABLE_TESTS=ON -DKYGX_ENABLE_DOCS=ON
cmake --build BuildHOS --config Release
cmake --build BuildHOS --target docs
cmake --install BuildHOS --prefix BuildHOS/Release
```

### libn3ds backend (baremetal)

- [Baremetal toolchain](https://github.com/kynex7510/ctr_bm_cmake_toolchain) required.

```sh
cmake -B BuildBM -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$CTR_BM_TOOLCHAIN_ROOT/Toolchain.cmake" -DCMAKE_BUILD_TYPE=Release -DKYGX_ENABLE_TESTS=ON -DKYGX_ENABLE_DOCS=ON
cmake --build BuildBM --config Release
cmake --build BuildBM --target docs
cmake --install BuildBM --prefix BuildBM/Release
```

Tested kernel configuration:

- `MAX_TASKS`: 4
- `MAX_EVENTS`: 16
- `MAX_MUTEXES`: 8
- `MAX_SEMAPHORES`: 10
- `MAX_TIMERS`: 0

## License

This library is doubly licensed:

- MPL 2.0, when linking with libctru (see [MPL_LICENSE.txt](MPL_LICENSE.txt)).
- GPLv3, when linking with libn3ds (see [GPL_LICENSE.txt](GPL_LICENSE.txt)).