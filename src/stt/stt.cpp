#include "stt.h"
#include "common-whisper.h"

#include <deque>
#include <print>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include <whisper.h>

STT::STT(whisper_context_params &cparams, whisper_full_params &wparams,
         string path_model, string language, bool no_context, Callback callback,
         QueueUpdateCallback queueCallback)
    : stopInference(false), whisper_callback(std::move(callback)),
      queue_update_callback(std::move(queueCallback)), cparams(cparams),
      path_model(std::move(path_model)), language(std::move(language)),
      wparams(wparams), no_context(no_context) {

  // wparams.language is just a pointer!
  this->wparams.language = this->language.c_str();
  spdlog::info("STT: wparams.language is {}", this->wparams.language);
  ctx = whisper_init_from_file_with_params(this->path_model.c_str(),
                                           this->cparams);

  if (!whisper_is_multilingual(ctx)) {
    if (this->language != "en" || this->wparams.translate) {
      this->language = "en";
      this->wparams.translate = false;
      println(stderr,
              "{}: WARNING: model is not multilingual, ignoring language "
              "and translation options",
              __func__);
    }
  }
}

STT::~STT() {
  whisper_print_timings(ctx);
  whisper_free(ctx);
}

auto STT::getQueueSizes() const -> vector<size_t> {
  lock_guard<mutex> lock(queueMutex);
  vector<size_t> sizes;
  queue<vector<float>> tempQueue = voiceQueue;

  while (!tempQueue.empty()) {
    sizes.push_back(tempQueue.front().size());
    tempQueue.pop();
  }
  return sizes;
}

void STT::notifyQueueUpdate() {
  if (queue_update_callback) {
    queue_update_callback(getQueueSizes());
  }
}

void STT::start() { processThread = thread(&STT::processVoices, this); }

void STT::processVoices() {
  while (true) {
    vector<float> mergedVoice;

    {
      unique_lock<mutex> lock(queueMutex);
      cv.wait(lock, [this]() {
        return (!voiceQueue.empty() && triggerMethod != NO_TRIGGER) ||
               stopInference;
      });
      spdlog::info(__func__, "processing voices");

      if (stopInference) {
        spdlog::info(__func__, "STT stop then return");
        return;
      }

      if (voiceQueue.empty()) {
        continue;
      }

      // Merge all available voices in the queue
      while (!voiceQueue.empty()) {
        vector<float> &voice = voiceQueue.front();
        mergedVoice.insert(mergedVoice.end(), voice.begin(), voice.end());
        voiceQueue.pop();
      }

      if (triggerMethod == ONCE_TRIGGER) {
        triggerMethod = NO_TRIGGER;
      }
    }

    notifyQueueUpdate();

    if (whisper_callback) {
      if (!mergedVoice.empty()) {
        string text = inference(mergedVoice);
        whisper_callback(text);
      } else {
        spdlog::error("{}: {}", __func__, "no voice data after merge");
      }
    }
  }
}

void STT::addVoice(vector<float> voice_data) {
  {
    lock_guard<mutex> lock(queueMutex);
    voiceQueue.push(voice_data);
  }
  cv.notify_one();
  notifyQueueUpdate();
}

void STT::stop() {
  {
    lock_guard<mutex> lock(queueMutex);
    stopInference = true;
  }
  cv.notify_all();
  processThread.join();
}

void STT::clearVoice() {
  {
    lock_guard<mutex> lock(queueMutex);
    queue<vector<float>> empty;
    swap(voiceQueue, empty);
  }
  notifyQueueUpdate();
}

auto STT::removeVoice(size_t index) -> bool {
  bool result = false;
  {
    lock_guard<mutex> lock(queueMutex);
    if (index >= voiceQueue.size())
      return false;

    // Convert queue to deque for random access
    deque<vector<float>> tempDeque;
    while (!voiceQueue.empty()) {
      tempDeque.push_back(voiceQueue.front());
      voiceQueue.pop();
    }

    tempDeque.erase(tempDeque.begin() + index);
    result = true;

    // Convert back to queue
    for (const auto &item : tempDeque) {
      voiceQueue.push(item);
    }
  }

  if (result) {
    notifyQueueUpdate();
  }
  return result;
}

void STT::setTriggerMethod(TriggerMethod triggerMethod) {
  {
    lock_guard<mutex> lock(queueMutex);
    this->triggerMethod = triggerMethod;
  }
  cv.notify_one();
}

auto STT::inference(vector<float> pcmf32) -> string {
  string result;
  spdlog::info("inference language is {}", language);
  spdlog::info("inference wparams.language is {}", wparams.language);

  wparams.prompt_tokens = no_context ? nullptr : prompt_tokens.data();
  wparams.prompt_n_tokens = no_context ? 0 : prompt_tokens.size();

  if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    return {};
  }

  {
    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
      const char *text = whisper_full_get_segment_text(ctx, i);

      result += text;
      if (i != n_segments - 1) {
        result += ",";
      }

      const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
      const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

      string output = "[" + to_timestamp(t0, false) + " --> " +
                      to_timestamp(t1, false) + "]  " + text;

      if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
        output += " [SPEAKER_TURN]";
      }

      spdlog::info("{}", output);
      fflush(stdout);
    }

    spdlog::info("### Transcription {} END", n_iter);
  }

  ++n_iter;

  if (!no_context) {
    prompt_tokens.clear();

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
      const int token_count = whisper_full_n_tokens(ctx, i);
      for (int j = 0; j < token_count; ++j) {
        prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
      }
    }
  }

  fflush(stdout);
  return result;
}
