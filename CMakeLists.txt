cmake_minimum_required(VERSION 3.10)
project(speakflow)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)
find_package(whisper REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED libcurl)
find_package(CLI11 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets)
add_subdirectory(dep/liboai)

file(GLOB SOURCES src/*.cpp res/*.qrc)

set(UI_FILES ui/mainwindow.ui)
set(MOC_HEADERS inc/mainwindow.h inc/document.h inc/previewpage.h)

qt6_wrap_ui(UI_HEADERS ${UI_FILES})
qt6_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})
# for .qrc
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME} ${SOURCES} ${UI_HEADERS} ${MOC_SOURCES})

target_include_directories(
  ${PROJECT_NAME} PRIVATE inc ${libcurl_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}
                          ${CMAKE_SOURCE_DIR}/dep/liboai/liboai/include)

target_link_libraries(
  ${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} whisper ${libcurl_LIBRARIES}
                          Qt6::Widgets Qt6::WebEngineWidgets oai)
