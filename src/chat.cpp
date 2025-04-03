#include "chat.h"
#include <thread>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// write callback function
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

Chat::Chat(whisper_params &params) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, params.url.c_str());

    token = params.token.c_str();
  }
}

Chat::~Chat() {
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void Chat::send_async(const std::string &input, Callback callback) {
  std::thread([this, input, callback]() {
    CURLcode res;
    std::string readBuffer;

    curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, token);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    json request_data = {{"model", "deepseek-chat"},
                         {
                             "messages",
                             json::array({
                                 {{"role", "user"}, {"content", input}},
                             }),
                         },
                         {"stream", false}};

    std::string json_data = request_data.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    // set callback function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      readBuffer =
          "Curl request failed: " + std::string(curl_easy_strerror(res));
    }

    if (callback) {
      callback(readBuffer);
    }

    curl_slist_free_all(headers);
  }).detach();
}
