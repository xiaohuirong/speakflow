cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(audio)
add_subdirectory(vad)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(STATUS "Building module_a standalone")
  add_subdirectory(../event event)
endif()

# Add source files
set(SEN_SOURCES sentense.cpp)

# Create library target
add_library(sentense STATIC ${SEN_SOURCES})

# Include directories
target_include_directories(sentense PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sentense PUBLIC audio_backend vad event)

option(BUILD_MODULE_TEST "Build module test executable" OFF)
# 条件性地添加测试可执行文件
if(BUILD_MODULE_TEST)
  add_executable(sentense_test ${SEN_SOURCES} test.cpp)
  target_link_libraries(sentense_test PRIVATE audio_backend vad event)
endif()
