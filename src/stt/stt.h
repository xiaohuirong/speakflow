#pragma once
#include "common_stt.h"
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <whisper.h>

using namespace std;

class STT {
public:
  using Callback = function<void(const string &)>;

  STT(whisper_context_params &cparams, whisper_full_params &wparams,
      string path_model, string language, bool no_context, Callback callback);

  ~STT();
  auto inference(vector<float> voice_data) -> string;

  void start();
  void stop();

  // Queue management functions
  auto getQueueSizes() const -> vector<size_t>;
  auto getOperations() -> STTOperations;

  void setCardWidgetCallbacks(const CardWidgetCallbacks &callbacks);

  void addVoice(vector<float> voice_data);
  auto removeVoice(size_t index) -> bool;
  void clearVoice();
  void setTriggerMethod(TriggerMethod triggerMethod);

private:
  void processVoices();

  Callback whisper_callback;
  CardWidgetCallbacks callbacks;

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
