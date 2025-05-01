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

set(FRONTBACK_SOURCES backend.cpp frontend.cpp main.cpp)
# Add the executable
add_executable(frontback ${FRONTBACK_SOURCES})

target_include_directories(frontback PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Link Qt6 libraries
target_link_libraries(frontback PRIVATE Qt6::Widgets)
