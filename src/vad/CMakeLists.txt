# Find required dependencies
find_package(onnxruntime REQUIRED)

# Add source files
set(VAD_SOURCES silero-vad-onnx.cpp)

# Create library target
add_library(vad STATIC ${VAD_SOURCES})

# Include directories
target_include_directories(vad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(vad PRIVATE onnxruntime::onnxruntime)
