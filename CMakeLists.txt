cmake_minimum_required(VERSION 3.10)
project(speakflow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)
find_package(whisper REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED libcurl)
find_package(nlohmann_json REQUIRED)
find_package(CLI11 REQUIRED)

file(GLOB SOURCES src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE inc ${libcurl_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} whisper
                      ${libcurl_LIBRARIES} nlohmann_json)
