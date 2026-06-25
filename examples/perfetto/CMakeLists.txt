cmake_minimum_required(VERSION 3.13)
project(pulseview_perfetto_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

set(PERFETTO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk")
if(NOT EXISTS "${PERFETTO_SDK_DIR}/perfetto.h")
  message(FATAL_ERROR
    "Perfetto SDK not found at ${PERFETTO_SDK_DIR}.\n"
    "Run ./fetch_sdk.sh in this directory, or download perfetto-cpp-sdk-src.zip\n"
    "from https://github.com/google/perfetto/releases/latest and extract here.")
endif()

add_library(perfetto_sdk STATIC "${PERFETTO_SDK_DIR}/perfetto.cc")
target_include_directories(perfetto_sdk PUBLIC "${PERFETTO_SDK_DIR}")

add_executable(pulseview_perfetto_example main.cc trace_categories.cc)
target_link_libraries(pulseview_perfetto_example PRIVATE perfetto_sdk Threads::Threads)

if(WIN32)
  target_compile_options(perfetto_sdk PRIVATE "/bigobj")
  target_compile_definitions(pulseview_perfetto_example PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(pulseview_perfetto_example PRIVATE ws2_32)
endif()
