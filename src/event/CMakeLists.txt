add_library(event INTERFACE)
target_include_directories(
  event INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} # 只需包含当前目录
)
