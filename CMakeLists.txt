cmake_minimum_required(VERSION 3.28.1)

set(CMAKE_GENERATOR_PLATFORM x64)

project(EldenRingGlee
  VERSION   "0.1.0"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

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

FetchContent_Declare(libdeflate
  GIT_REPOSITORY  https://github.com/ebiggers/libdeflate.git
  GIT_TAG         v1.22
)

FetchContent_MakeAvailable(
  spdlog
  pugixml
  libdeflate)

add_library(EldenRingGlee SHARED
  src/dllmain.cpp
)

set_target_properties(EldenRingGlee PROPERTIES OUTPUT_NAME "GleeRecolorer")

target_link_libraries(EldenRingGlee spdlog)

add_definitions(-DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
