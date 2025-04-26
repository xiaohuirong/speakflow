cmake_minimum_required(VERSION 3.10)
project(speakflow)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(whisper QUIET)
if(NOT whisper_FOUND)
  message(STATUS "whisper not found, using submodule in dep/whisper")
  set(GGML_CUDA
      1
      CACHE BOOL "Enable CUDA for GGML")
  add_subdirectory(dep/whisper)
  set(WHISPER_LIB whisper)
  set(WHISPER_INCLUDE_DIR dep/whisper/include)
else()
  set(WHISPER_LIB whisper)
  set(WHISPER_INCLUDE_DIR ${whisper_INCLUDE_DIRS})
endif()

find_package(liboai QUIET)
if(NOT liboai_FOUND)
  message(STATUS "liboai not found, using submodule in dep/liboai")
  add_subdirectory(dep/liboai)
  set(LIBOAI_LIB oai)
  set(LIBOAI_INCLUDE_DIR dep/liboai/liboai/include)
else()
  set(LIBOAI_LIB oai)
  set(LIBOAI_INCLUDE_DIR ${liboai_INCLUDE_DIRS})
endif()

# Find CLI11
find_package(CLI11 QUIET)
if(NOT CLI11_FOUND)
  message(STATUS "CLI11 not found, using submodule in dep/cli11")
  add_subdirectory(dep/cli11)
  set(CLI11_INCLUDE_DIR dep/cli11/include)
else()
  set(CLI11_INCLUDE_DIR ${CLI11_INCLUDE_DIRS})
endif()

find_package(SDL2 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Multimedia Charts)
find_package(spdlog REQUIRED)

file(GLOB SOURCES src/*.cpp res/*.qrc)
set(UI_FILES ui/mainwindow.ui ui/monitorwindow.ui)
set(MOC_HEADERS inc/mainwindow.h inc/document.h inc/previewpage.h
                inc/messagequeue.h inc/qtaudio.h inc/monitorwindow.h)

qt6_wrap_ui(UI_HEADERS ${UI_FILES})
qt6_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})
# for .qrc
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME} ${SOURCES} ${UI_HEADERS} ${MOC_SOURCES})
target_include_directories(
  ${PROJECT_NAME} PRIVATE inc ${CMAKE_BINARY_DIR} ${WHISPER_INCLUDE_DIR}
                          ${LIBOAI_INCLUDE_DIR} ${CLI11_INCLUDE_DIR})
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE ${WHISPER_LIB}
          ${LIBOAI_LIB}
          ${SDL2_LIBRARIES}
          spdlog
          fmt
          Qt6::Widgets
          Qt6::WebEngineWidgets
          Qt6::Multimedia
          Qt6::Charts)
