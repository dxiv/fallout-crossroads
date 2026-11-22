cmake_minimum_required(VERSION 3.24)

# Fail before language enablement so Linux/macOS hosts get a clear message
# instead of a broken compiler probe (this project is Win32/MSVC-only).
if(NOT WIN32)
  message(FATAL_ERROR
    "Fallout: Crossroads Phase 0 targets Windows only (Win32 + MSVC). "
    "Configure this project on Windows 10/11 with Visual Studio 2022 or Build Tools.")
endif()

project(fallout-crossroads
  VERSION 0.0.1
  DESCRIPTION "Fallout: Crossroads — Phase 0 boot launcher"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(crossroads_boot WIN32
  src/main.cpp
  src/app.cpp
  src/log.cpp
  src/settings.cpp
  src/install_detect.cpp
  src/legal.cpp
  src/app.rc
  src/app.manifest
)

set_source_files_properties(src/app.manifest PROPERTIES HEADER_FILE_ONLY TRUE)

target_include_directories(crossroads_boot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(crossroads_boot PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  CROSSROADS_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
  target_compile_options(crossroads_boot PRIVATE /W4 /permissive- /Zc:__cplusplus)
  # UTF-8 source and execution charset
  target_compile_options(crossroads_boot PRIVATE /utf-8)
endif()

target_link_libraries(crossroads_boot PRIVATE
  comctl32
  comdlg32
  shell32
  ole32
  version
  shlwapi
)

set_target_properties(crossroads_boot PROPERTIES
  OUTPUT_NAME "FalloutCrossroads"
  WIN32_EXECUTABLE TRUE
)

install(TARGETS crossroads_boot RUNTIME DESTINATION bin)
