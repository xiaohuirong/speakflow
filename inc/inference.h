#pragma once
#include "parse.h"
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <whisper.h>

class S2T {
public:
  using Callback = std::function<void(const std::string &)>;

  struct Voice {
    std::vector<float> voice_data;
    bool no_context;
  };

  S2T(whisper_params &params, Callback callback);
  ~S2T();
  auto inference(bool no_context, std::vector<float> voice_data) -> std::string;

  void start();
  void stop();
  void addVoice(bool no_context, std::vector<float> voice_data);

private:
  void processVoices();

  Callback whisper_callback;

  std::queue<Voice> voiceQueue; // Message queue
  bool stopInference;           // Whether to stop the voice system

  std::mutex queueMutex;      // Mutex to protect the message queue
  std::condition_variable cv; // Condition variable for thread synchronization

  std::thread processThread;

  whisper_full_params wparams;
  int n_iter = 0;
  whisper_context_params cparams = whisper_context_default_params();
  whisper_context *ctx;

  std::vector<whisper_token> prompt_tokens;
};
