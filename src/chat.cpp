#include "chat.h"
#include "liboai.h"
#include <ostream>
#include <print>
#include <thread>

using namespace liboai;

Chat::Chat(whisper_params &params) : stopChat(false) {

  oai = new OpenAI(params.url);

  if (!oai->auth.SetKey(params.token)) {
    std::println("auth failed!");
  }
}

void Chat::start() { chatThread = std::thread(&Chat::processMessages, this); }

void Chat::stop() {
  {
    std::lock_guard<std::mutex> lock(queueMutex);
    stopChat = true;
  }
  cv.notify_all();
  chatThread.join();
}

void Chat::sendMessage(const std::string &messageText, Callback callback) {
  {
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push({messageText, callback});
  }
  cv.notify_one();
}

void Chat::processMessages() {
  while (true) {
    Message message = {.text = "", .callback = nullptr};

    {
      std::unique_lock<std::mutex> lock(queueMutex);
      cv.wait(lock, [this]() { return !messageQueue.empty() || stopChat; });

      if (stopChat && messageQueue.empty()) {
        break;
      }

      message = messageQueue.front();
      messageQueue.pop();
    }

    std::cout << "Processing message: " << message.text << std::endl;

    if (message.callback) {
      std::string response = wait_response(message.text);
      // callback
      message.callback(response);
    }
  }
}

auto Chat::wait_response(const std::string input) -> std::string {
  // add a message to the conversation
  if (!convo.AddUserData(input)) {
    return "This is a error: add a message failed";
  }

  try {
    auto fut = oai->ChatCompletion->create_async("deepseek-chat", convo);

    // check if the future is ready
    fut.wait();

    // get the contained response
    auto response = fut.get();

    // update our conversation with the response
    if (!convo.Update(response)) {
      std::println("update conversation failed");
      return "This is a error: update conversation failed";
    }

    // print the response
    std::cout << convo.GetLastResponse() << std::endl;

    return convo.GetLastResponse();
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return "This is a error: try fail";
  }
}
