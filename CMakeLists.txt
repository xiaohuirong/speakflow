cmake_minimum_required(VERSION 3.10)
project(stream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)

find_package(whisper REQUIRED)

set(SOURCES src/main.cpp src/common-sdl.cpp src/common.cpp
            src/common-whisper.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE inc)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} whisper)
