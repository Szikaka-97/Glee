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

FetchContent_MakeAvailable(
  spdlog
  pugixml)

add_library(EldenRingGlee SHARED
  src/dllmain.cpp
)

set_target_properties(EldenRingGlee PROPERTIES OUTPUT_NAME "GleeRecolorer")

target_link_libraries(EldenRingGlee spdlog)

add_definitions(-DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

add_custom_command(TARGET EldenRingGlee POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:EldenRingGlee> "C:/Users/Szikaka/Desktop/Modowanie/Elden Ring/ModEngine-2.1.0.0-win64/mod/Glee/GleeRecolorer.dll"
)