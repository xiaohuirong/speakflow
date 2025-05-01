// common_interface.h - 前后端共享的接口定义
#ifndef COMMON_INTERFACE_H
#define COMMON_INTERFACE_H

#include <functional>
#include <string>
#include <vector>

// 前端需要实现的回调接口
struct FrontendCallbacks {
  // 添加项目回调
  std::function<void(const std::string &item)> onItemAdded;
  // 移除项目回调
  std::function<void(const std::string &item)> onItemRemoved;
  // 清空列表回调
  std::function<void()> onListCleared;
};

// 后端操作接口
struct BackendOperations {
  // 添加项目
  std::function<void(const std::string &item)> addItem;
  // 移除项目
  std::function<void(const std::string &item)> removeItem;
  // 获取所有项目
  std::function<std::vector<std::string>()> getAllItems;
  // 清空列表
  std::function<void()> clearList;
};

#endif // COMMON_INTERFACE_H
