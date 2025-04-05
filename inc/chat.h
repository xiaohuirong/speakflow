#pragma once

#include "liboai.h"
#include "parse.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace liboai;

class Chat {
public:
  using Callback = std::function<void(const std::string &, bool)>;

  struct Message {
    std::string text;
    Callback callback;
  };

  Chat(whisper_params &params);

  void start();
  void stop();
  void addMessage(const std::string &messageText, Callback callback);

private:
  void processMessages();
  auto wait_response(const std::string input) -> std::string;

  std::queue<Message> messageQueue; // Message queue
  std::mutex queueMutex;            // Mutex to protect the message queue
  std::condition_variable cv; // Condition variable for thread synchronization
  bool stopChat;              // Whether to stop the chat system
  std::thread chatThread;     // Chat system thread

  OpenAI *oai;
  Conversation convo;
  std::string key;
  int message_count = 0;
};
