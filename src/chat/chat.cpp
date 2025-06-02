#include "chat.h"
#include "events.h"
#include "liboai.h"
#include <print>
#include <spdlog/spdlog.h>
#include <thread>
#include <utility>

Chat::Chat(string url, string key, string model, int32_t timeout, string system,
           std::shared_ptr<EventBus> bus)
    : stopChat(false), key(key), model(std::move(model)), url(url),
      timeout(timeout), system(system), eventBus(std::move(bus)) {

  oai = new OpenAI(url);

  if (!oai->auth.SetKey(key)) {
    spdlog::error("auth failed!");
  }

  oai->auth.SetMaxTimeout(timeout);

  if (system != "") {
    if (!convo.SetSystemData(system)) {
      spdlog::error("set system data failed");
    }
  }

  eventBus->subscribe<StartServiceEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto startEvent = std::static_pointer_cast<StartServiceEvent>(event);
        if (startEvent->serviceName == "chat") {
          start();
          eventBus->publish<ServiceStatusEvent>("chat", true);
        }
      });

  eventBus->subscribe<StopServiceEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto stopEvent = std::static_pointer_cast<StopServiceEvent>(event);
        if (stopEvent->serviceName == "chat") {
          stop();
          eventBus->publish<ServiceStatusEvent>("chat", false);
        }
      });

  eventBus->subscribe<MessageAddedEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto messageEvent = std::static_pointer_cast<MessageAddedEvent>(event);
        if (messageEvent->serviceName == "stt") {
          string keyword = "明镜与点点";
          if (messageEvent->message != "" &&
              messageEvent->message.find(keyword) == string::npos) {
            addMessage(messageEvent->message);
          }
        }
      });
}

void Chat::start() { chatThread = thread(&Chat::processMessages, this); }

void Chat::stop() {
  {
    lock_guard<mutex> lock(queueMutex);
    stopChat = true;
  }
  cv.notify_all();
  chatThread.join();
}

void Chat::addMessage(const string &messageText) {
  {
    lock_guard<mutex> lock(queueMutex);
    messageQueue.push({messageText});
  }
  cv.notify_one();
}

void Chat::processMessages() {
  while (true) {
    string message = "";

    {
      unique_lock<mutex> lock(queueMutex);
      cv.wait(lock, [this]() { return !messageQueue.empty() || stopChat; });

      if (stopChat) {
        return;
      }

      while (!messageQueue.empty()) {
        message += messageQueue.front();
        messageQueue.pop();
      }

      eventBus->publish<MessageClearedEvent>();
    }

    spdlog::info("Processing message: {0}", message);
    message_count += 1;

    eventBus->publish<MessageAddedEvent>(
        "chat", format("## USER {} \n", message_count) + message);

    string response = wait_response(message);
    // callback

    eventBus->publish<MessageAddedEvent>(
        "chat", format("## AI {} \n", message_count) + response);
  }
}

auto Chat::wait_response(const string input) -> string {
  // add a message to the conversation
  if (!convo.AddUserData(input)) {
    return "This is a error: add a message failed";
  }

  try {
    auto fut = oai->ChatCompletion->create_async(model, convo);

    // check if the future is ready
    fut.wait();

    // get the contained response
    auto response = fut.get();

    // update our conversation with the response
    if (!convo.Update(response)) {
      spdlog::error("update conversation failed");
      return "This is a error: update conversation failed";
    }

    // print the response
    spdlog::info("AI responce is: {0}", convo.GetLastResponse());

    return convo.GetLastResponse();
  } catch (std::exception &e) {
    spdlog::error(e.what());
    return "This is a error: try fail";
  }
}
