# Find required dependencies Find CLI11
find_package(CLI11 QUIET)
if(NOT CLI11_FOUND)
  message(STATUS "CLI11 not found, using submodule in dep/cli11")
  add_subdirectory(../../dep/cli11)
  set(CLI11_INCLUDE_DIR ../../dep/cli11/include)
else()
  set(CLI11_INCLUDE_DIR ${CLI11_INCLUDE_DIRS})
endif()

find_package(spdlog REQUIRED)

# Add source files
set(PARSE_SOURCES parse.cpp)

# Create library target
add_library(parse STATIC ${PARSE_SOURCES})

# Include directories
target_include_directories(parse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                        ${CLI11_INCLUDE_DIR})

# Link libraries
target_link_libraries(parse PUBLIC fmt spdlog)
