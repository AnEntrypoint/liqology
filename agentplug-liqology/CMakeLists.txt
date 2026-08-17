cmake_minimum_required(VERSION 3.25)
project(agentplug_liqology LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    message(FATAL_ERROR "agentplug-liqology only builds for the wasm32-wasip1-threads triplet")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FAISS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/faiss")
set(FAISS_CORE_SRC
    "${FAISS_ROOT}/faiss/IndexFlat.cpp"
    "${FAISS_ROOT}/faiss/IndexFlatCodes.cpp"
    "${FAISS_ROOT}/faiss/Index.cpp"
    "${FAISS_ROOT}/faiss/impl/FaissException.cpp"
    "${FAISS_ROOT}/faiss/impl/AuxIndexStructures.cpp"
    "${FAISS_ROOT}/faiss/impl/CodePacker.cpp"
    "${FAISS_ROOT}/faiss/impl/IDSelector.cpp"
    "${FAISS_ROOT}/faiss/utils/distances.cpp"
    "${FAISS_ROOT}/faiss/utils/distances_simd.cpp"
    "${FAISS_ROOT}/faiss/utils/distances_fused/distances_fused.cpp"
    "${FAISS_ROOT}/faiss/utils/extra_distances.cpp"
    "${FAISS_ROOT}/faiss/utils/simd_levels.cpp"
    "${FAISS_ROOT}/faiss/utils/partitioning.cpp"
    "${FAISS_ROOT}/faiss/utils/sorting.cpp"
)

add_library(agentplug_liqology_faiss_core STATIC ${FAISS_CORE_SRC})
target_include_directories(agentplug_liqology_faiss_core PUBLIC "${FAISS_ROOT}")
target_include_directories(agentplug_liqology_faiss_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/wasm-shims")
target_compile_options(agentplug_liqology_faiss_core PRIVATE -pthread)

add_library(agentplug_liqology_wasm_shims STATIC
    wasm-shims/blas_shim.cpp
    wasm-shims/lapack_unreachable_stubs.cpp
    wasm-shims/omp_shim.cpp
)
target_include_directories(agentplug_liqology_wasm_shims PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/wasm-shims")

add_executable(agentplug_liqology
    src/abi.cpp
    src/dispatch.cpp
)
set_target_properties(agentplug_liqology PROPERTIES SUFFIX ".wasm")
target_include_directories(agentplug_liqology PRIVATE
    src
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann"
)
target_link_libraries(agentplug_liqology PRIVATE
    agentplug_liqology_faiss_core
    agentplug_liqology_wasm_shims
    Microsoft.GSL::GSL
)
target_compile_options(agentplug_liqology PRIVATE -pthread)
target_compile_definitions(agentplug_liqology PRIVATE JSON_NOEXCEPTION)
target_link_options(agentplug_liqology PRIVATE
    -pthread
    -Wl,--no-entry
    -Wl,--export=plugkit_alloc
    -Wl,--export=plugkit_free
    -Wl,--export=plugin_call
)
