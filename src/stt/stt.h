#pragma once
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <whisper.h>

using namespace std;

class STT {
public:
  using Callback = function<void(const string &)>;

  struct Voice {
    vector<float> voice_data;
    bool no_context;
  };

  STT(whisper_context_params &cparams, whisper_full_params &wparams,
      string path_model, Callback callback);
  ~STT();
  auto inference(bool no_context, vector<float> voice_data) -> string;

  void start();
  void stop();
  void addVoice(bool no_context, vector<float> voice_data);

private:
  void processVoices();

  Callback whisper_callback;

  queue<Voice> voiceQueue; // Message queue
  bool stopInference;      // Whether to stop the voice system

  mutex queueMutex;      // Mutex to protect the message queue
  condition_variable cv; // Condition variable for thread synchronization

  thread processThread;

  whisper_full_params wparams;
  int n_iter = 0;
  whisper_context_params cparams;
  whisper_context *ctx;

  vector<whisper_token> prompt_tokens;
};
