#include "chat.h"
#include "liboai.h"
#include <ostream>
#include <print>
#include <thread>

Chat::Chat(whisper_params &params, Callback callback) : stopChat(false) {

  whisper_callback = callback;

  oai = new OpenAI(params.url);

  key = params.token;
  model = params.llm;

  if (!oai->auth.SetKey(key)) {
    println("auth failed!");
  }

  oai->auth.SetMaxTimeout(params.timeout);

  if (params.system != "") {
    if (!convo.SetSystemData(params.system)) {
      println("set system data failed");
    }
  }
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
    Message message = {.text = ""};

    {
      unique_lock<mutex> lock(queueMutex);
      cv.wait(lock, [this]() { return !messageQueue.empty() || stopChat; });

      if (stopChat) {
        return;
      }

      if (messageQueue.empty()) {
        continue;
      }

      message = messageQueue.front();
      messageQueue.pop();
    }

    cout << "Processing message: " << message.text << endl;
    message_count += 1;

    whisper_callback(format("## USER {} \n", message_count) + message.text,
                     false);

    if (whisper_callback) {
      string response = wait_response(message.text);
      // callback
      whisper_callback(format("## AI {} \n", message_count) + response, true);
    }
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
      println("update conversation failed");
      return "This is a error: update conversation failed";
    }

    // print the response
    cout << convo.GetLastResponse() << endl;

    return convo.GetLastResponse();
  } catch (std::exception &e) {
    cout << e.what() << endl;
    return "This is a error: try fail";
  }
}
