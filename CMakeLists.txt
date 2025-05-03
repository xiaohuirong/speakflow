cmake_minimum_required(VERSION 3.10)
project(speakflow)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src/event)

add_subdirectory(src/sentense)
add_subdirectory(src/chat)
add_subdirectory(src/stt)
add_subdirectory(src/parse)
add_subdirectory(src/widgets/queman)
add_subdirectory(src/widgets/cardman)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Charts)
find_package(spdlog REQUIRED)

file(GLOB SOURCES src/*.cpp res/*.qrc)
set(UI_FILES ui/mainwindow.ui ui/monitorwindow.ui)
set(MOC_HEADERS inc/mainwindow.h inc/document.h inc/previewpage.h
                inc/monitorwindow.h)

qt6_wrap_ui(UI_HEADERS ${UI_FILES})
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} ${SOURCES} ${UI_HEADERS} ${MOC_HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE inc ${CMAKE_BINARY_DIR})
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE spdlog
          fmt
          Qt6::Widgets
          Qt6::WebEngineWidgets
          Qt6::Charts
          sentense
          stt
          chat
          parse
          event
          cardman)
