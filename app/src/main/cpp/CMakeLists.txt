cmake_minimum_required(VERSION 3.22.1)
project(audioengine)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  oboe
  GIT_REPOSITORY https://github.com/google/oboe.git
  GIT_TAG 1.8.1
)
FetchContent_MakeAvailable(oboe)

add_library(audioengine SHARED
  core/audio_engine.cpp
  jni/jni_bridge.cpp
)

target_include_directories(audioengine PRIVATE
  ${oboe_SOURCE_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/core
  ${CMAKE_CURRENT_LIST_DIR}/dsp
)

target_link_libraries(audioengine
  oboe
  log
  android
)
