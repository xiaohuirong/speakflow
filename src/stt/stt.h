#pragma once
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <whisper.h>

using namespace std;

class STT {
public:
  enum TriggerMethod { AUTO_TRIGGER = -1, NO_TRIGGER = 0, ONCE_TRIGGER = 1 };

  using Callback = function<void(const string &)>;
  using QueueUpdateCallback = function<void(const vector<size_t> &)>;

  STT(whisper_context_params &cparams, whisper_full_params &wparams,
      string path_model, string language, bool no_context, Callback callback,
      QueueUpdateCallback queueCallback);

  ~STT();
  auto inference(vector<float> voice_data) -> string;

  void start();
  void stop();
  void addVoice(vector<float> voice_data);

  // Queue management functions
  void clearVoice();
  auto removeVoice(size_t index) -> bool;
  void setTriggerMethod(TriggerMethod triggerMethod);

  auto getQueueSizes() const -> vector<size_t>;

private:
  void processVoices();
  void notifyQueueUpdate();

  Callback whisper_callback;
  QueueUpdateCallback queue_update_callback;

  queue<vector<float>> voiceQueue; // Message queue
  bool stopInference;              // Whether to stop the voice system
  TriggerMethod triggerMethod = AUTO_TRIGGER;

  mutable mutex queueMutex; // Mutex to protect the message queue
  condition_variable cv;    // Condition variable for thread synchronization

  thread processThread;

  whisper_full_params wparams;
  string language;
  string path_model;
  int n_iter = 0;
  whisper_context_params cparams;
  whisper_context *ctx;

  bool no_context = false;

  vector<whisper_token> prompt_tokens;
};
