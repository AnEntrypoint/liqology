set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME WASI)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../cmake/wasip1-threads.toolchain.cmake")

set(VCPKG_DISABLE_COMPILER_TRACKING ON)

# wasip1-threads.toolchain.cmake reads WASI_SDK_PREFIX from the environment;
# vcpkg's port-build subprocess does not inherit the invoking shell's
# environment unless the variable is explicitly passed through.
set(VCPKG_ENV_PASSTHROUGH WASI_SDK_PREFIX)
