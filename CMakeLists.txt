cmake_minimum_required(VERSION 3.22.1)

if (WIN32)
  set(CMAKE_GENERATOR_PLATFORM x64)
endif (WIN32)

project(EldenRingGlee
  VERSION   "0.1.0"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include(FetchContent)

if (WIN32)
  set(SPDLOG_DISABLE_DEFAULT_LOGGER ON)
endif (WIN32)

set(SPDLOG_USE_STD_FORMAT ON)
FetchContent_Declare(spdlog
  GIT_REPOSITORY  https://github.com/gabime/spdlog.git
  GIT_TAG         v1.13.0
)

FetchContent_Declare(pugixml
  GIT_REPOSITORY  https://github.com/zeux/pugixml.git
  GIT_TAG         v1.14
)

set(ZLIB_COMPAT ON)
set(ZLIB_ENABLE_TESTS OFF)
set(ZLIBNG_ENABLE_TESTS OFF)
set(WITH_SANITIZER OFF)
set(WITH_GTEST OFF)
FetchContent_Declare(zlib
  GIT_REPOSITORY  https://github.com/zlib-ng/zlib-ng.git
  GIT_TAG         2.2.2
)

FetchContent_MakeAvailable(
  spdlog
  pugixml
  zlib
)

if (WIN32)
  add_library(EldenRingGlee SHARED
    src/dllmain.cpp
    src/binary.cpp
    src/bnd_file.cpp
    src/dcx_file.cpp
    src/matbin_file.cpp
    src/compression.cpp
    src/utils.cpp
    src/logging.cpp
    src/mat/material_mod.cpp
    src/mat/material_change.cpp
  )
else(WIN32)
  add_executable(EldenRingGlee
    src/dllmain.cpp
    src/binary.cpp
    src/bnd_file.cpp
    src/dcx_file.cpp
    src/matbin_file.cpp
    src/compression.cpp
    src/utils.cpp
    src/logging.cpp
    src/mat/material_mod.cpp
    src/mat/material_change.cpp
  )

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif(WIN32)

include_directories(src/mat)

set_target_properties(EldenRingGlee PROPERTIES OUTPUT_NAME "GleeRecolorer")

target_link_libraries(EldenRingGlee spdlog pugixml zlib)

add_definitions(-DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
