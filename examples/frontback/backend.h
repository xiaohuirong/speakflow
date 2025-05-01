// backend.h - 纯C++后端实现
#ifndef BACKEND_H
#define BACKEND_H

#include "common_interface.h"
#include <deque>
#include <mutex>
#include <string>

class Backend {
public:
  Backend();
  ~Backend();

  // 初始化后端操作接口
  auto getOperations() -> BackendOperations;

  // 设置前端回调
  void setFrontendCallbacks(const FrontendCallbacks &callbacks);

private:
  std::deque<std::string> itemQueue;
  std::mutex queueMutex;
  FrontendCallbacks frontendCallbacks;

  void notifyItemAdded(const std::string &item);
  void notifyItemRemoved(const std::string &item);
  void notifyListCleared();
};

#endif // BACKEND_H
