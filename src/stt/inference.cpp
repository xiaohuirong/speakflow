#include "inference.h"
#include "common-whisper.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <print>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <whisper.h>

S2T::S2T(whisper_context_params &cparams, whisper_full_params &wparams,
         string path_model, Callback callback)
    : stopInference(false), whisper_callback(callback), cparams(cparams),
      wparams(wparams) {

  ctx = whisper_init_from_file_with_params(path_model.c_str(), cparams);

  // if (!whisper_is_multilingual(ctx)) {
  //   if (params.language != "en" || params.translate) {
  //     params.language = "en";
  //     params.translate = false;
  //     println(stderr,
  //             "{}: WARNING: model is not multilingual, ignoring language "
  //             "and translation options",
  //             __func__);
  //   }
  // }
}

S2T::~S2T() {
  whisper_print_timings(ctx);
  whisper_free(ctx);
}

void S2T::start() { processThread = thread(&S2T::processVoices, this); }

void S2T::processVoices() {
  while (true) {
    Voice voice = {.voice_data = vector<float>(), .no_context = false};

    {
      unique_lock<mutex> lock(queueMutex);
      cv.wait(lock, [this]() { return !voiceQueue.empty() || stopInference; });
      spdlog::info(__func__, "pick a voice");

      if (stopInference) {
        spdlog::info(__func__, "S2T stop then return");
        return;
      }

      if (voiceQueue.empty()) {
        continue;
      }

      voice = voiceQueue.front();
      voiceQueue.pop();
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

void S2T::addVoice(bool no_context, vector<float> voice_data) {
  {
    lock_guard<mutex> lock(queueMutex);
    voiceQueue.push({voice_data, no_context});
  }
  cv.notify_one();
}

void S2T::stop() {
  {
    lock_guard<mutex> lock(queueMutex);
    stopInference = true;
  }
  cv.notify_all();
  processThread.join();
}

static ofstream fout;

// duration = (t_last - t_start).count()
auto S2T::inference(bool no_context, vector<float> pcmf32) -> string
// run the inference
{
  string result;

  wparams.prompt_tokens = no_context ? nullptr : prompt_tokens.data();
  wparams.prompt_n_tokens = no_context ? 0 : prompt_tokens.size();

  if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    // fprintf(stderr, "%s: failed to process audio\n", argv[0]);
    return {};
  }

  // print result;
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

  // Add tokens of the last full length segment as the prompt
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
