cmake_minimum_required(VERSION 3.10)
# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(STATUS "Building module_a standalone")
  add_subdirectory(../event event)
endif()

# Find required dependencies
find_package(whisper QUIET)
if(NOT whisper_FOUND)
  message(STATUS "whisper not found, using submodule in dep/whisper")
  add_subdirectory(../../dep/whisper whisper)
endif()

find_package(spdlog REQUIRED)

# Add source files
set(STT_SOURCES stt.cpp common-whisper.cpp)

# Create library target
add_library(stt STATIC ${STT_SOURCES})

# Include directories
target_include_directories(stt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(stt PUBLIC fmt spdlog whisper event)

if(BUILD_MODULE_TEST)
  # Add the executable
  add_executable(stt_test test.cpp ${STT_SOURCES})
  # Link Qt6 libraries
  target_link_libraries(stt_test PRIVATE fmt spdlog whisper event)
endif()
