add_subdirectory(event)
add_subdirectory(sentense)
add_subdirectory(chat)
add_subdirectory(stt)
add_subdirectory(parse)
add_subdirectory(widgets/queman)
add_subdirectory(widgets/cardman)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Charts)
find_package(spdlog REQUIRED)

file(GLOB SOURCES ./*.cpp ../res/*.qrc)
set(UI_FILES mainwindow.ui monitorwindow.ui)
set(MOC_HEADERS mainwindow.h document.h previewpage.h
                monitorwindow.h)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} ${SOURCES} ${UI_FILES} ${MOC_HEADERS})
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
