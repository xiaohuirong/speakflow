# audio/CMakeLists.txt

# 定义音频后端选项
set(USE_AUDIO
    "SDL"
    CACHE STRING "Audio backend to use (SDL or QT)")

# 设置有效的选项值
set_property(CACHE USE_AUDIO PROPERTY STRINGS SDL QT)

# 检查选项值是否有效
if(NOT USE_AUDIO STREQUAL "SDL" AND NOT USE_AUDIO STREQUAL "QT")
  message(FATAL_ERROR "Invalid audio backend '${USE_AUDIO}'. Must be SDL or QT")
endif()

# 根据选择添加相应的源文件
if(USE_AUDIO STREQUAL "SDL")
  find_package(SDL2 REQUIRED)
  add_library(audio_backend STATIC common-sdl.cpp)
  # PUBLIC let another file can use this *.h
  target_include_directories(audio_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                  ${SDL2_INCLUDE_DIRS})
  target_link_libraries(audio_backend PRIVATE SDL2)
  target_compile_definitions(audio_backend PUBLIC USE_SDL_AUDIO=1)
  message(STATUS "Using SDL audio backend")

elseif(USE_AUDIO STREQUAL "QT")
  find_package(Qt6 REQUIRED COMPONENTS Multimedia)
  set(CMAKE_AUTOMOC ON)
  add_library(audio_backend STATIC qtaudio.cpp)
  target_include_directories(audio_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(audio_backend PRIVATE Qt6::Multimedia)
  target_compile_definitions(audio_backend PUBLIC USE_QT_AUDIO=1)
  message(STATUS "Using Qt audio backend")
endif()
