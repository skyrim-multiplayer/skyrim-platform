cmake_minimum_required(VERSION 3.17)
project(skyrim_multiplayer)

option(SKYRIM_DIR "Path to Skyrim" OFF)
if (NOT SKYRIM_DIR)
  message(FATAL_ERROR "Please specify SKYRIM_DIR")
endif()
if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
  message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
endif()

option(CEF_DIR "Path to CEF dist" OFF)
if (NOT CEF_DIR)
  message(FATAL_ERROR "Please specify CEF_DIR")
endif()

set(expected_readme_hash 231a23f26745365bb630bd6ba98ebdf50fa78051e3885468badcfa0af91c935ebec059c5ecffc6be389d7daabacf3f8f56f8a13747f86ec6f5c3caaa8a9354cc)
file(SHA512 ${CEF_DIR}/README.txt readme_hash)
if (NOT "${readme_hash}" STREQUAL expected_readme_hash)
  message(FATAL_ERROR "Unexpected CEF README hash\n\tExpected: ${expected_readme_hash}\n\tActual: ${readme_hash}")
endif()
list(APPEND CMAKE_MODULE_PATH "${CEF_DIR}/cmake")

include(src/cmake/cmake_generate.cmake)

# Clear CMake cache after changing this
# (also for subprojects like platform_se)
set(VCPKG_REVISION cb8a9fe7157f8ad2423742f893955ed9ced05910)

# Download vcpkg dependencies
include(pmm.cmake)
set(ports
  chakracore:x64-windows
  nlohmann-json:x64-windows
  cpp-httplib:x64-windows
  zlib:x64-windows
  mimalloc:x64-windows
)
foreach(port ${ports})
  message(STATUS "Installing ${port}")
  pmm(VCPKG REVISION ${VCPKG_REVISION} REQUIRES ${port})
endforeach()
if ("${_VCPKG_ROOT_DIR}" STREQUAL "")
  message(FATAL_ERROR "Expected _VCPKG_ROOT_DIR variable to be set by pmm")
endif()

cmake_generate(
  NAME platform_se
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/platform_se"
  BINARY_DIR "${CMAKE_BINARY_DIR}/_platform_se"
  ARCHITECTURE "x64"
  GENERATOR "${CMAKE_GENERATOR}"
  VARIABLES
    "SKYRIM_MP_ROOT=${CMAKE_SOURCE_DIR}"
    "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "VCPKG_ROOT=${_VCPKG_ROOT_DIR}"
    "CMAKE_TOOLCHAIN_FILE=${_VCPKG_ROOT_DIR}\\scripts\\buildsystems\\vcpkg.cmake"
    "SKYRIM_DIR=${SKYRIM_DIR}"
    "CEF_DIR=${CEF_DIR}"
)
