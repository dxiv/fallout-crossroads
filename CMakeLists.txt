cmake_minimum_required(VERSION 3.24)

if(NOT WIN32)
  message(FATAL_ERROR
    "Fallout: Crossroads targets Windows only (Win32 + MSVC). "
    "Configure on Windows 10/11 with Visual Studio 2022 or Build Tools.")
endif()

project(fallout-crossroads
  VERSION 0.5.0
  DESCRIPTION "Fallout: Crossroads"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CROSSROADS_COMMON_DEFS
  UNICODE
  _UNICODE
  NOMINMAX
  CROSSROADS_VERSION="${PROJECT_VERSION}"
)

function(crossroads_msvc_options target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
  endif()
endfunction()

# Shared helpers (settings, logging, install checks, ESM probe, host launch)
add_library(crossroads_core STATIC
  src/log.cpp
  src/settings.cpp
  src/install_detect.cpp
  src/esm_probe.cpp
  src/esm_index.cpp
  src/host_launch.cpp
  src/esp_writer.cpp
  src/cell_import.cpp
  src/world_import.cpp
  src/bsa_wire.cpp
  src/capital_travel.cpp
  src/transit_esp.cpp
  src/transit_journey.cpp
  src/mojave_foothold.cpp
  src/coverage_policy.cpp
  src/spine_esp.cpp
  src/crash_guard.cpp
  src/doctor.cpp
  src/bridge_foothold.cpp
)
target_include_directories(crossroads_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(crossroads_core PUBLIC ${CROSSROADS_COMMON_DEFS})
crossroads_msvc_options(crossroads_core)
target_link_libraries(crossroads_core PUBLIC version shlwapi shell32 ole32)

# Boot launcher UI
add_executable(crossroads_boot WIN32
  src/main.cpp
  src/app.cpp
  src/legal.cpp
  src/app.rc
  src/app.manifest
)
set_source_files_properties(src/app.manifest PROPERTIES HEADER_FILE_ONLY TRUE)
target_link_libraries(crossroads_boot PRIVATE
  crossroads_core
  comctl32
  comdlg32
  shell32
  ole32
)
crossroads_msvc_options(crossroads_boot)
set_target_properties(crossroads_boot PROPERTIES
  OUTPUT_NAME "FalloutCrossroads"
  WIN32_EXECUTABLE TRUE
)

# In-process plugin loaded into FalloutNV.exe
add_library(crossroads_plugin SHARED
  src/plugin/dllmain.cpp
  src/plugin/plugin.cpp
)
target_link_libraries(crossroads_plugin PRIVATE crossroads_core)
crossroads_msvc_options(crossroads_plugin)
set_target_properties(crossroads_plugin PROPERTIES
  OUTPUT_NAME "CrossroadsPlugin"
  PREFIX ""
)

# Put EXE + DLL next to each other for local runs
set_target_properties(crossroads_boot crossroads_plugin PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)

install(TARGETS crossroads_boot crossroads_plugin
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
)
