Package: sqlite3[core,json1]:x64-windows@3.54.1

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.29.30159.0
-    vcpkg-tool version: 2024-12-09-1005b78fa1bf1dde1a20c2734cba4ea61ca94d9a
    vcpkg-readonly: true
    vcpkg-scripts version: 0c4cf19224a049cf82f4521e29e39f7bd680440c

**To Reproduce**

`vcpkg install `

**Failure logs**

```
-- Using cached sqlite-autoconf-3540100.tar.gz.
-- Extracting source C:/Users/ASUS/AppData/Local/vcpkg/downloads/sqlite-autoconf-3540100.tar.gz
-- Applying patch fix-arm-uwp.patch
-- Applying patch add-config-include.patch
-- Using source at C:/Users/ASUS/Documents/Visual Studio 2022/TraidingORM/vcpkg_installed/vcpkg/blds/sqlite3/src/nf-3540100-0b4b6fce90.clean
-- Found external ninja('1.12.1').
CMake Error at scripts/cmake/vcpkg_host_path_list.cmake:60 (message):
  Operation REMOVE_DUPLICATES not recognized.
Call Stack (most recent call first):
  C:/Users/ASUS/Documents/Visual Studio 2022/TraidingORM/vcpkg_installed/x64-windows/share/vcpkg-cmake/vcpkg_cmake_configure.cmake:246 (vcpkg_host_path_list)
  C:/Users/ASUS/AppData/Local/vcpkg/registries/git-trees/603ae9e17b08b01ee35b063c4c5862896986942e/portfile.cmake:71 (vcpkg_cmake_configure)
  scripts/ports.cmake:196 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "trading-orm",
  "version-string": "1.0.0",
  "dependencies": [
    "sqlite3",
    "sqlite-orm"
  ],
  "builtin-baseline": "8f363bdc11359ede4bc51e5eb32a2617538049c8"
}

```
</details>
