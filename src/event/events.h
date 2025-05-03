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

class MessageAddedEvent : public Event {
public:
  std::string dataType;
  std::vector<float> message;

  MessageAddedEvent(std::string type, std::vector<float> msg)
      : dataType(std::move(type)), message(std::move(msg)) {}
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
