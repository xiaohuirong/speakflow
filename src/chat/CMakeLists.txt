# Find required dependencies
find_package(liboai QUIET)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(STATUS "Building module_a standalone")
  add_subdirectory(../event event)
endif()

if(NOT liboai_FOUND)
  message(STATUS "liboai not found, using submodule in dep/liboai")
  add_subdirectory(../../dep/liboai liboai)
endif()

find_package(spdlog REQUIRED)

# Add source files
set(CHAT_SOURCES chat.cpp)

# Create library target
add_library(chat STATIC ${CHAT_SOURCES})

# Include directories
target_include_directories(chat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(chat PUBLIC fmt spdlog oai event)
