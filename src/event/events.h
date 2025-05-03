#pragma once
#include "eventbus.h"
#include <cstddef>
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
  std::vector<float> audio;
  AudioAddedEvent(std::vector<float> audio_data)
      : audio(std::move(audio_data)) {}
};

class AudioRemovedEvent : public Event {
public:
  size_t index;
  AudioRemovedEvent(size_t i) : index(i) {}
};

class AudioClearedEvent : public Event {
public:
  AudioClearedEvent() = default;
};

class AudioSentEvent : public Event {
public:
  AudioSentEvent() = default;
};

class MessageAddedEvent : public Event {
public:
  std::string serviceName;
  std::string message;

  MessageAddedEvent(std::string name, std::string msg)
      : serviceName(std::move(name)), message(std::move(msg)) {}
};

class AutoModeSetEvent : public Event {
public:
  std::string serviceName;
  bool isAutoMode;

  AutoModeSetEvent(std::string name, bool is_auto)
      : serviceName(std::move(name)), isAutoMode(is_auto) {}
};

// 错误事件
class ErrorOccurredEvent : public Event {
public:
  std::string message;
  int errorCode;

  ErrorOccurredEvent(std::string msg, int code)
      : message(std::move(msg)), errorCode(code) {}
};

class StartServiceEvent : public Event {
public:
  std::string serviceName;
  explicit StartServiceEvent(std::string name) : serviceName(std::move(name)) {}
};

class StopServiceEvent : public Event {
public:
  std::string serviceName;
  explicit StopServiceEvent(std::string name) : serviceName(std::move(name)) {}
};

class ServiceStatusEvent : public Event {
public:
  std::string serviceName;
  bool isRunning;
  ServiceStatusEvent(std::string name, bool running)
      : serviceName(std::move(name)), isRunning(running) {}
};
