#pragma once
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <vector>

class Event {
public:
  virtual ~Event() = default;
};

using EventHandler = std::function<void(const std::shared_ptr<Event> &)>;

class EventBus {
  std::map<std::type_index, std::vector<EventHandler>> handlers;
  std::mutex mutex;

public:
  template <typename EventType> void subscribe(EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex);
    handlers[typeid(EventType)].emplace_back(std::move(handler));
  }

  template <typename EventType, typename... Args> void publish(Args &&...args) {
    auto event = std::make_shared<EventType>(std::forward<Args>(args)...);
    std::vector<EventHandler> localHandlers;

    {
      std::lock_guard<std::mutex> lock(mutex);
      auto it = handlers.find(typeid(EventType));
      if (it != handlers.end()) {
        localHandlers = it->second;
      }
    }

    for (auto &handler : localHandlers) {
      handler(event);
    }
  }
};
