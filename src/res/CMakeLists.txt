find_package(Qt6 REQUIRED COMPONENTS Core)

set(CMAKE_AUTORCC ON)

set(RESOURCE_FILES markdowneditor.qrc)

add_library(resources STATIC ${RESOURCE_FILES})
target_link_libraries(resources PRIVATE Qt6::Core)
