cmake_minimum_required(VERSION 3.10)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Enable automatic moc, uic, and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CARDMAN_SOURCES cardman.h cardman.cpp flowlayout.cpp)
add_library(cardman STATIC ${cardman_SOURCES})
# Include directories
target_include_directories(cardman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cardman PUBLIC Qt6::Widgets)

option(BUILD_MODULE_TEST "Build module test executable" OFF)
# 条件性地添加测试可执行文件
if(BUILD_MODULE_TEST)
  # Add the executable
  add_executable(cardman_test test.cpp ${CARDMAN_SOURCES})
  # Link Qt6 libraries
  target_link_libraries(cardman_test PRIVATE Qt6::Widgets)
endif()
