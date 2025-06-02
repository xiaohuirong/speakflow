find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOUIC ON)

set(UI_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/monitorwindow.ui
)

add_library(ui INTERFACE)
target_sources(ui INTERFACE ${UI_FILES})
target_link_libraries(ui INTERFACE Qt6::Widgets)
target_include_directories(ui INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
