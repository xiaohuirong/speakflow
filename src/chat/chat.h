#pragma once

#include "liboai.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace liboai;
using namespace std;

class Chat {
public:
  using Callback = function<void(const string &, bool)>;

  struct Message {
    string text;
  };

  Chat(string url, string key, string model, int32_t timeout, string system,
       Callback callback);

  void start();
  void stop();
  void addMessage(const string &messageText);

private:
  void processMessages();
  auto wait_response(const string input) -> string;

  Callback whisper_callback;

  queue<Message> messageQueue; // Message queue
  mutex queueMutex;            // Mutex to protect the message queue
  condition_variable cv;       // Condition variable for thread synchronization
  bool stopChat;               // Whether to stop the chat system
  thread chatThread;           // Chat system thread

  OpenAI *oai;
  Conversation convo;
  string key;
  string url;
  int message_count = 0;

  string model;

  int32_t timeout;

  string system;
};
