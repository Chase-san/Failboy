# Toolchain: the plain clang driver + ld.lld against a MinGW-w64 sysroot.
# No MSVC headers, libraries or linker are involved.
#
#   cmake --preset clang-mingw
#   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-mingw.cmake -DMINGW_ROOT=C:/mingw64

set(MINGW_ROOT "C:/mingw64" CACHE PATH "MinGW-w64 installation used as the sysroot")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MINGW_ROOT)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER_TARGET x86_64-w64-mingw32)
set(CMAKE_CXX_COMPILER_TARGET x86_64-w64-mingw32)
set(CMAKE_SYSROOT "${MINGW_ROOT}")
set(CMAKE_RC_COMPILER "${MINGW_ROOT}/bin/windres.exe")
set(CMAKE_LINKER_TYPE LLD)
