// backend.cpp
#include "backend.h"

Backend::Backend() {
  // 初始化空的回调
  frontendCallbacks.onItemAdded = nullptr;
  frontendCallbacks.onItemRemoved = nullptr;
  frontendCallbacks.onListCleared = nullptr;
}

Backend::~Backend() {}

BackendOperations Backend::getOperations() {
  BackendOperations ops;

  ops.addItem = [this](const std::string &item) {
    std::lock_guard<std::mutex> lock(queueMutex);
    itemQueue.push_back(item);
    notifyItemAdded(item);
  };

  ops.removeItem = [this](const std::string &item) {
    std::lock_guard<std::mutex> lock(queueMutex);
    for (auto it = itemQueue.begin(); it != itemQueue.end(); ++it) {
      if (*it == item) {
        itemQueue.erase(it);
        notifyItemRemoved(item);
        break;
      }
    }
  };

  ops.getAllItems = [this]() -> std::vector<std::string> {
    std::lock_guard<std::mutex> lock(queueMutex);
    return std::vector<std::string>(itemQueue.begin(), itemQueue.end());
  };

  ops.clearList = [this]() {
    std::lock_guard<std::mutex> lock(queueMutex);
    itemQueue.clear();
    notifyListCleared();
  };

  return ops;
}

void Backend::setFrontendCallbacks(const FrontendCallbacks &callbacks) {
  frontendCallbacks = callbacks;
}

void Backend::notifyItemAdded(const std::string &item) {
  if (frontendCallbacks.onItemAdded) {
    frontendCallbacks.onItemAdded(item);
  }
}

void Backend::notifyItemRemoved(const std::string &item) {
  if (frontendCallbacks.onItemRemoved) {
    frontendCallbacks.onItemRemoved(item);
  }
}

void Backend::notifyListCleared() {
  if (frontendCallbacks.onListCleared) {
    frontendCallbacks.onListCleared();
  }
}
