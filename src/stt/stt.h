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
      string path_model, string language, Callback callback);

  ~STT();
  auto inference(bool no_context, vector<float> voice_data) -> string;

  void start();
  void stop();
  void addVoice(bool no_context, vector<float> voice_data);

  // Queue management functions
  void clearQueue();
  auto removeFromQueue(size_t index) -> bool;
  auto mergeInQueue(size_t startIndex, size_t count) -> bool;
  auto moveInQueue(size_t index, int distance) -> bool;
  auto moveToFront(size_t index) -> bool;
  auto moveToBack(size_t index) -> bool;

  // Manual trigger control
  void setAutoProcessing(bool autoProcess);
  auto processNext() -> bool;
  auto processAt(size_t index) -> bool;
  auto queueSize() -> size_t;

private:
  void processVoices();

  Callback whisper_callback;

  queue<Voice> voiceQueue; // Message queue
  bool stopInference;      // Whether to stop the voice system
  int autoProcessing =
      -1; // -1 : infinity time, 0: not process, > 0: process time

  mutex queueMutex;      // Mutex to protect the message queue
  condition_variable cv; // Condition variable for thread synchronization

  thread processThread;

  whisper_full_params wparams;
  string language;
  string path_model;
  int n_iter = 0;
  whisper_context_params cparams;
  whisper_context *ctx;

  vector<whisper_token> prompt_tokens;
};
