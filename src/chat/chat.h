#pragma once

#include "liboai.h"

#include "eventbus.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace liboai;
using namespace std;

class Chat {
public:
  Chat(string url, string key, string model, int32_t timeout, string system,
       std::shared_ptr<EventBus> bus);

private:
  void addMessage(const string &messageText);

  void start();
  void stop();

  void processMessages();
  auto wait_response(const string input) -> string;

  std::shared_ptr<EventBus> eventBus;

  queue<string> messageQueue; // Message queue
  mutex queueMutex;           // Mutex to protect the message queue
  condition_variable cv;      // Condition variable for thread synchronization
  bool stopChat;              // Whether to stop the chat system
  thread chatThread;          // Chat system thread

  OpenAI *oai;
  Conversation convo;
  string key;
  string url;
  int message_count = 0;

  string model;

  int32_t timeout;

  string system;
};
