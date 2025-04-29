# Find required dependencies
find_package(whisper QUIET)
if(NOT whisper_FOUND)
  message(STATUS "whisper not found, using submodule in dep/whisper")
  set(GGML_CUDA
      1
      CACHE BOOL "Enable CUDA for GGML")
  add_subdirectory(../../dep/whisper)
  set(WHISPER_LIB whisper)
  set(WHISPER_INCLUDE_DIR dep/whisper/include)
else()
  set(WHISPER_LIB whisper)
  set(WHISPER_INCLUDE_DIR ${whisper_INCLUDE_DIRS})
endif()

find_package(spdlog REQUIRED)

# Add source files
set(STT_SOURCES stt.cpp common.cpp common-whisper.cpp)

# Create library target
add_library(stt STATIC ${STT_SOURCES})

# Include directories
target_include_directories(stt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                      ${WHISPER_INCLUDE_DIR})

# Link libraries
target_link_libraries(stt PUBLIC fmt spdlog ${LIBOAI_LIB} ${WHISPER_LIB})
