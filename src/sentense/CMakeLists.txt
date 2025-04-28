add_subdirectory(audio)
add_subdirectory(vad)

# Add source files
set(SEN_SOURCES sentense.cpp)

# Create library target
add_library(sentense STATIC ${SEN_SOURCES})

# Include directories
target_include_directories(sentense PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sentense PUBLIC audio_backend vad)
