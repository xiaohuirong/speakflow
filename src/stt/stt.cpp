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
         string path_model, string language, Callback callback)
    : stopInference(false), whisper_callback(std::move(callback)),
      cparams(cparams), path_model(std::move(path_model)),
      language(std::move(language)), wparams(wparams) {

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

void STT::start() { processThread = thread(&STT::processVoices, this); }

void STT::processVoices() {
  while (true) {
    Voice voice = {.voice_data = vector<float>(), .no_context = false};

    {
      unique_lock<mutex> lock(queueMutex);
      cv.wait(lock, [this]() {
        return (!voiceQueue.empty() && autoProcessing != 0) || stopInference;
      });
      spdlog::info(__func__, "pick a voice");

      if (stopInference) {
        spdlog::info(__func__, "STT stop then return");
        return;
      }

      if (voiceQueue.empty()) {
        continue;
      }

      voice = voiceQueue.front();
      voiceQueue.pop();

      if (autoProcessing > 0) {
        autoProcessing -= 1;
      }
    }

    if (whisper_callback) {
      if (!voice.voice_data.empty()) {
        string text = inference(voice.no_context, voice.voice_data);
        whisper_callback(text);
      } else {
        spdlog::error(__func__, "no voice");
      }
    }
  }
}

void STT::addVoice(bool no_context, vector<float> voice_data) {
  {
    lock_guard<mutex> lock(queueMutex);
    voiceQueue.push({voice_data, no_context});
  }
  if (autoProcessing) {
    cv.notify_one();
  }
}

void STT::stop() {
  {
    lock_guard<mutex> lock(queueMutex);
    stopInference = true;
  }
  cv.notify_all();
  processThread.join();
}

// Queue management functions
void STT::clearQueue() {
  lock_guard<mutex> lock(queueMutex);
  queue<Voice> empty;
  swap(voiceQueue, empty);
}

auto STT::removeFromQueue(size_t index) -> bool {
  lock_guard<mutex> lock(queueMutex);
  if (index >= voiceQueue.size())
    return false;

  // Convert queue to deque for random access
  deque<Voice> tempDeque;
  while (!voiceQueue.empty()) {
    tempDeque.push_back(voiceQueue.front());
    voiceQueue.pop();
  }

  tempDeque.erase(tempDeque.begin() + index);

  // Convert back to queue
  for (const auto &item : tempDeque) {
    voiceQueue.push(item);
  }

  return true;
}

auto STT::mergeInQueue(size_t startIndex, size_t count) -> bool {
  lock_guard<mutex> lock(queueMutex);
  if (startIndex >= voiceQueue.size() || startIndex + count > voiceQueue.size())
    return false;

  deque<Voice> tempDeque;
  while (!voiceQueue.empty()) {
    tempDeque.push_back(voiceQueue.front());
    voiceQueue.pop();
  }

  if (count < 2)
    return true; // Nothing to merge

  // Merge the specified items
  vector<float> mergedData;
  bool no_context = tempDeque[startIndex].no_context;

  for (size_t i = startIndex; i < startIndex + count; ++i) {
    mergedData.insert(mergedData.end(), tempDeque[i].voice_data.begin(),
                      tempDeque[i].voice_data.end());
  }

  // Remove the merged items
  tempDeque.erase(tempDeque.begin() + startIndex,
                  tempDeque.begin() + startIndex + count);

  // Insert the merged item
  tempDeque.insert(tempDeque.begin() + startIndex, {mergedData, no_context});

  // Convert back to queue
  for (const auto &item : tempDeque) {
    voiceQueue.push(item);
  }

  return true;
}

auto STT::moveInQueue(size_t index, int distance) -> bool {
  lock_guard<mutex> lock(queueMutex);
  if (index >= voiceQueue.size())
    return false;

  deque<Voice> tempDeque;
  while (!voiceQueue.empty()) {
    tempDeque.push_back(voiceQueue.front());
    voiceQueue.pop();
  }

  size_t newPos = index + distance;
  if (distance < 0 && newPos > index) { // Underflow check
    newPos = 0;
  } else if (newPos >= tempDeque.size()) {
    newPos = tempDeque.size() - 1;
  }

  if (newPos == index)
    return true; // No movement needed

  Voice item = tempDeque[index];
  tempDeque.erase(tempDeque.begin() + index);
  tempDeque.insert(tempDeque.begin() + newPos, item);

  // Convert back to queue
  for (const auto &item : tempDeque) {
    voiceQueue.push(item);
  }

  return true;
}

auto STT::moveToFront(size_t index) -> bool {
  return moveInQueue(index, -static_cast<int>(index));
}

auto STT::moveToBack(size_t index) -> bool {
  return moveInQueue(index, voiceQueue.size() - index - 1);
}

// Manual trigger control
void STT::setAutoProcessing(bool autoProcess) {
  {
    lock_guard<mutex> lock(queueMutex);
    autoProcessing = autoProcess;
  }
  if (autoProcess) {
    cv.notify_one();
  }
}

auto STT::processNext() -> bool {
  {
    lock_guard<mutex> lock(queueMutex);
    if (voiceQueue.empty())
      return false;

    if (autoProcessing >= 0) {
      autoProcessing += 1;
    }
  }

  cv.notify_one();

  return true;
}

auto STT::processAt(size_t index) -> bool {
  {
    lock_guard<mutex> lock(queueMutex);
    if (index >= voiceQueue.size())
      return false;

    deque<Voice> tempDeque;
    while (!voiceQueue.empty()) {
      tempDeque.push_back(voiceQueue.front());
      voiceQueue.pop();
    }

    Voice item = tempDeque[index];
    tempDeque.erase(tempDeque.begin() + index);
    tempDeque.insert(tempDeque.begin(), item); // 移动到最前面

    // Convert back to queue
    for (const auto &item : tempDeque) {
      voiceQueue.push(item);
    }

    if (autoProcessing >= 0) {
      autoProcessing += 1;
    }
  }

  return true;
}

auto STT::queueSize() -> size_t {
  lock_guard<mutex> lock(queueMutex);
  return voiceQueue.size();
}

auto STT::inference(bool no_context, vector<float> pcmf32) -> string {
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
