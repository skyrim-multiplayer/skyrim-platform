cmake_minimum_required(VERSION 3.17)
if ("${CMAKE_PROJECT_NAME}" STREQUAL "" OR "${CMAKE_PROJECT_NAME}" STREQUAL "platform_se")
  set(CMAKE_SYSTEM_VERSION "10.0.17763.0" CACHE STRING INTERNAL FORCE)
  set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.17763.0" CACHE STRING INTERNAL FORCE)
  project(platform_se)
endif()

if ("${SKYRIM_MP_ROOT}" STREQUAL "")
  message(FATAL_ERROR "SKYRIM_MP_ROOT is not specified")
endif()

set(cmake_dir "${SKYRIM_MP_ROOT}/src/cmake")
if (NOT EXISTS "${cmake_dir}")
  message(FATAL_ERROR "\"${cmake_dir}\" does not exist")
endif()

if ("${CEF_DIR}" STREQUAL "")
  message(FATAL_ERROR "CEF_DIR is not specified")
endif()
set(CEF_ROOT ${CEF_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
set(CEF_LIBRARIES
  ${CEF_PATH}/$<CONFIG>/cef_sandbox.lib
  ${CEF_PATH}/$<CONFIG>/libcef.lib
  libcef_dll_wrapper
)
set(CEF_INCLUDE
  ${CEF_DIR}
)

set(lib_root "${SKYRIM_MP_ROOT}/submodules/CommonLibSSE")
set(directxtex_root "${SKYRIM_MP_ROOT}/submodules/DirectXTex")
set(third_party "${SKYRIM_MP_ROOT}/src/platform_se/third_party")

set(BUILD_DX12 OFF CACHE BOOL "Build with DirectX12 Runtime support" FORCE)
add_subdirectory(${directxtex_root} DirectXTex)

file(GLOB_RECURSE lib_src "${lib_root}/src/*" "${lib_root}/include/*")
add_library(CommonLibSSE STATIC ${lib_src})
target_include_directories(CommonLibSSE PUBLIC "${lib_root}/include" "${third_party}")
target_compile_options(CommonLibSSE PUBLIC "/FI\"ForceInclude.h\"" "/FI\"SKSE/Logger.h\"")
target_link_libraries(CommonLibSSE PUBLIC Version)

file(GLOB_RECURSE common_src "${third_party}/common/*")
add_library(common STATIC ${common_src})
target_compile_options(common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(common PRIVATE "${third_party}/common" "${third_party}")

file(GLOB_RECURSE skse64_common_src "${third_party}/skse64_common/*")
add_library(skse64_common STATIC ${skse64_common_src})
target_compile_options(skse64_common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(skse64_common PRIVATE "${third_party}/common" "${third_party}/skse64_common" "${third_party}")

file(GLOB_RECURSE skse_src "${third_party}/skse64/*")
add_library(skse64 STATIC ${skse_src})
target_compile_options(skse64 PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(skse64 PUBLIC "${third_party}/common" "${third_party}/skse64" "${third_party}")
target_compile_definitions(skse64 PRIVATE RUNTIME=1 RUNTIME_VERSION=0x01050610)
target_link_libraries(skse64 PUBLIC common skse64_common)

include(${cmake_dir}/apply_default_settings.cmake)
include(${cmake_dir}/integrate_vcpkg.cmake)

file(GLOB_RECURSE core_library_src "${third_party}/cef/core_library/*")
add_library(core_library STATIC ${core_library_src})
target_include_directories(core_library PUBLIC "${third_party}/cef/core_library")
integrate_vcpkg(
  TARGETS core_library
  VCPKG_PATH "${VCPKG_ROOT}"
)

file(GLOB_RECURSE ui_src "${third_party}/cef/ui/*")
add_library(ui STATIC ${ui_src})
target_link_libraries(ui PUBLIC ${CEF_LIBRARIES} core_library)
target_include_directories(ui PUBLIC ${CEF_INCLUDE})
target_include_directories(ui PRIVATE ${third_party}/cef/ui ${directxtex_root})
integrate_vcpkg(
  TARGETS ui
  VCPKG_PATH "${VCPKG_ROOT}"
)

if (NOT "${SKIP_SKYRIM_PLATFORM_BUILDING}")

  include(${cmake_dir}/CMakeRC.cmake)

  # assets
  set(ASSET_FILES ${CMAKE_SOURCE_DIR}/skyrim_platform/savefile/template.ess)
    cmrc_add_resource_library(skyrim_plugin_resources ${ASSET_FILES}
    ALIAS assets NAMESPACE skyrim_plugin_resources WHENCE skyrim_platform
  )
  apply_default_settings(TARGETS skyrim_plugin_resources)

  include(${cmake_dir}/npm.cmake)

  set(wdir "${CMAKE_CURRENT_BINARY_DIR}/_codegen")
  file(MAKE_DIRECTORY ${wdir})
  npm_execute_command(
    WORKING_DIRECTORY ${wdir}
    COMMAND init -y
  )
  npm_execute_command(
    WORKING_DIRECTORY ${wdir}
    COMMAND install tsc-watch@4.2.3 typescript@3.8.3 --save
  )

  file(GLOB_RECURSE platform_src "skyrim_platform/*" "${SKYRIM_MP_ROOT}/.clang-format")
  add_library(skyrim_platform SHARED ${platform_src})
  target_link_libraries(skyrim_platform PUBLIC skse64 CommonLibSSE skyrim_plugin_resources DirectXTex)
  set_target_properties(skyrim_platform PROPERTIES OUTPUT_NAME "SkyrimPlatform")
  target_include_directories(skyrim_platform PRIVATE "${third_party}")
  target_link_libraries(skyrim_platform PRIVATE "${third_party}/frida/frida-gum.lib")

  apply_default_settings(TARGETS skyrim_platform)

  integrate_vcpkg(
    TARGETS skyrim_platform
    VCPKG_PATH "${VCPKG_ROOT}"
  )

  set_target_properties(skyrim_platform PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
    PDB_OUTPUT_DIRECTORY "bin"
  )

  function(add_papyrus_library)
    set(options)
    set(oneValueArgs NAME DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(A
      "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    file(GLOB src ${A_DIRECTORY}/*.psc)

    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/pex)
    add_custom_target(${A_NAME} ALL
      COMMAND "${SKYRIM_DIR}\\Papyrus Compiler\\PapyrusCompiler.exe"
        ${A_DIRECTORY} -output=${OUTPUT_DIR} -import=${A_DIRECTORY} -all
      SOURCES ${src}
    )
  endfunction()

  add_papyrus_library(NAME TESModPlatformPsc DIRECTORY "${CMAKE_SOURCE_DIR}/psc")
  add_dependencies(TESModPlatformPsc skyrim_platform)

  add_custom_command(
    TARGET TESModPlatformPsc POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/touch_$<CONFIG>
  )
  add_custom_target(codegen ALL
    COMMAND node ${CMAKE_SOURCE_DIR}/codegen/index.js
    WORKING_DIRECTORY ${wdir}
  )
endif()

apply_default_settings(TARGETS
  CommonLibSSE common skse64_common skse64
  core_library ui DirectXTex texdiag texconv texassemble
)
