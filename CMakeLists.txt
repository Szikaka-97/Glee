cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_GENERATOR_PLATFORM x64)

project(EldenRingGlee
  VERSION   "0.1.0"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# The common prefix path when downloading zlib from GnuWin
# https://gnuwin32.sourceforge.net/packages/zlib.htm
# If you get it from a different source, or build or linux you'll have to change that
set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/GnuWin32")

include(FetchContent)

set(SPDLOG_DISABLE_DEFAULT_LOGGER ON)
set(SPDLOG_USE_STD_FORMAT ON)
FetchContent_Declare(spdlog
  GIT_REPOSITORY  https://github.com/gabime/spdlog.git
  GIT_TAG         v1.13.0
)

FetchContent_Declare(pugixml
  GIT_REPOSITORY  https://github.com/zeux/pugixml.git
  GIT_TAG         v1.14
)

FetchContent_MakeAvailable(
  spdlog
  pugixml
)

add_library(EldenRingGlee SHARED
  src/dllmain.cpp
  src/binary.cpp
  src/bnd_file.cpp
  src/dcx_file.cpp
  src/matbin_file.cpp
)

set_target_properties(EldenRingGlee PROPERTIES OUTPUT_NAME "GleeRecolorer")

find_package(ZLIB REQUIRED)
target_link_libraries(EldenRingGlee spdlog pugixml ZLIB::ZLIB)

add_definitions(-DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
