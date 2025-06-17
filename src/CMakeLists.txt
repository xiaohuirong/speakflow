add_subdirectory(event)
add_subdirectory(sentense)
add_subdirectory(chat)
add_subdirectory(stt)
add_subdirectory(parse)
add_subdirectory(widgets/queman)
add_subdirectory(widgets/cardman)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Charts)
find_package(spdlog REQUIRED)

file(GLOB SOURCES *.cpp *.ui res/*.qrc)
set(MOC_HEADERS mainwindow.h document.h previewpage.h monitorwindow.h)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME} ${SOURCES} ${MOC_HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR})
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
