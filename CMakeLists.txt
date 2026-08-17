cmake_minimum_required(VERSION 3.25)
project(liqology LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(Boost CONFIG COMPONENTS container QUIET)
find_package(tl-expected CONFIG QUIET)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    find_package(faiss CONFIG REQUIRED)
endif()

add_library(liqology INTERFACE)
add_library(liqology::liqology ALIAS liqology)

target_include_directories(liqology INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(liqology INTERFACE Microsoft.GSL::GSL)
if(Boost_FOUND)
    target_link_libraries(liqology INTERFACE Boost::container)
endif()
if(tl-expected_FOUND)
    target_link_libraries(liqology INTERFACE tl::expected)
    target_compile_definitions(liqology INTERFACE LIQOLOGY_HAS_TL_EXPECTED=1)
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    target_link_libraries(liqology INTERFACE faiss)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(liqology INTERFACE -Wall -Wextra -Wpedantic)
endif()

option(LIQOLOGY_BUILD_EXAMPLES "Build liqology example programs" ON)
if(LIQOLOGY_BUILD_EXAMPLES)
    add_subdirectory(examples/hello)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
        add_subdirectory(examples/memory-loop)
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "WASI" AND CMAKE_CXX_COMPILER_TARGET MATCHES "wasip1-threads")
    add_subdirectory(agentplug-liqology)
endif()

include(GNUInstallDirs)
install(DIRECTORY include/liqology DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS liqology EXPORT liqologyTargets)
install(EXPORT liqologyTargets FILE liqologyTargets.cmake NAMESPACE liqology:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/liqology)
