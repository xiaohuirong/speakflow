// common_interface.h - 前后端共享的接口定义
#ifndef COMMON_INTERFACE_H
#define COMMON_INTERFACE_H

#include <functional>
#include <string>
#include <vector>

enum TriggerMethod { AUTO_TRIGGER = -1, NO_TRIGGER = 0, ONCE_TRIGGER = 1 };

// 前端需要实现的回调接口
struct CardWidgetCallbacks {
  // 添加项目回调
  std::function<void(const std::string &length)> onVoiceAdded;

  // 移除项目回调
  std::function<void(const size_t index)> onVoiceRemoved;

  // 清空列表回调
  std::function<void()> onVoiceCleared;

  // 触发方法改变回调
  std::function<void(const TriggerMethod TriggerMethod)> onTriggerMethodChanged;
};

// 后端操作接口
struct STTOperations {
  // 添加项目
  std::function<void(const std::vector<float> &voice)> addVoice;

  // 移除项目
  std::function<bool(const size_t index)> removeVoice;

  // 清空列表
  std::function<void()> clearVoice;

  // 设置触发方法
  std::function<void(const TriggerMethod triggerMethod)> setTriggerMethod;
};

#endif // COMMON_INTERFACE_H
