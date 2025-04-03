#pragma once

#include "parse.h"

#include <curl/curl.h>
#include <functional>
#include <string>

class Chat {
public:
  Chat(whisper_params &params);
  ~Chat();

  using Callback = std::function<void(const std::string &)>;
  void send_async(const std::string &input, Callback callback);

private:
  CURL *curl;
  const char *token;
};
