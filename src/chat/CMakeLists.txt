# Find required dependencies
find_package(liboai QUIET)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  message(STATUS "Building module_a standalone")
  add_subdirectory(../event event)
endif()

if(NOT liboai_FOUND)
  message(STATUS "liboai not found, using submodule in dep/liboai")
  add_subdirectory(../../dep/liboai ../../dep/liboai)
  set(LIBOAI_LIB oai)
  set(LIBOAI_INCLUDE_DIR dep/liboai/liboai/include)
else()
  set(LIBOAI_LIB oai)
  set(LIBOAI_INCLUDE_DIR ${liboai_INCLUDE_DIRS})
endif()

find_package(spdlog REQUIRED)

# Add source files
set(CHAT_SOURCES chat.cpp)

# Create library target
add_library(chat STATIC ${CHAT_SOURCES})

# Include directories
target_include_directories(chat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                       ${LIBOAI_INCLUDE_DIR})

# Link libraries
target_link_libraries(chat PUBLIC fmt spdlog ${LIBOAI_LIB} event)
