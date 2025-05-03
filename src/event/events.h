#pragma once
#include "eventbus.h"
#include <string>

// 数据更新事件
class DataUpdatedEvent : public Event {
public:
  std::string dataType;
  int newValue;

  DataUpdatedEvent(std::string type, int value)
      : dataType(std::move(type)), newValue(value) {}
};

class AudioAddedEvent : public Event {
public:
  std::string dataType;
  std::vector<float> audio;

  AudioAddedEvent(std::string type, std::vector<float> audio_data)
      : dataType(std::move(type)), audio(std::move(audio_data)) {}
};

// 错误事件
class ErrorOccurredEvent : public Event {
public:
  std::string message;
  int errorCode;

  ErrorOccurredEvent(std::string msg, int code)
      : message(std::move(msg)), errorCode(code) {}
};
