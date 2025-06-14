# KYGX

Low level abstraction of the GX graphics system for the 3DS.

## Setup

Download a [prebuilt](https://github.com/kynex7510/KYGX/releases) version, use as a CMake dependency, or build manually.

### HOS build (userland)

```sh
cmake -B BuildHOS -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release -DKYGX_ENABLE_TESTS=ON
cmake --build BuildHOS --config Release
cmake --install BuildHOS --prefix BuildHOS/Release
```

### Baremetal build

```sh
cmake -B BuildBM -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="$(pwd)/CMake/Toolchain.cmake" -DCMAKE_BUILD_TYPE=Release -DKYGX_BAREMETAL=ON -DKYGX_ENABLE_TESTS=ON
cmake --build BuildBM --config Release
cmake --install BuildBM --prefix BuildBM/Release
```

- [firmtool](https://github.com/TuxSH/firmtool) is required.
- `LIBN3DS_ROOT` must be set accordingly, either as an environment variable, or as a CMake variable.

Tested kernel configuration:

- `MAX_TASKS`: 12
- `MAX_EVENTS`: 32
- `MAX_MUTEXES`: 16
- `MAX_SEMAPHORES`: 8
- `MAX_TIMERS`: 0

## Usage

Read the [docs](DOCS.md) to understand the GX subsystem, and to have a quick view on how to use the library. Additionally, the [tests](Tests) folder includes some examples.

## License

This library is doubly licensed:

- MIT, for HOS (userland) usage (see [HOS_LICENSE.txt](HOS_LICENSE.txt)).
- GPLv3, for baremetal usage (see [BM_LICENSE.txt](BM_LICENSE.txt)).