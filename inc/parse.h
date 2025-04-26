#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>

using namespace std;

// command-line parameters
struct whisper_params {
  int32_t n_threads = min(4, (int32_t)thread::hardware_concurrency());
  int32_t step_ms = 3000;
  int32_t length_ms = 10000;
  int32_t keep_ms = 200;
  int32_t capture_id = -1;
  int32_t max_tokens = 32;
  int32_t audio_ctx = 0;
  int32_t beam_size = -1;

  int32_t n_samples_keep = 0;
  int32_t n_samples_step = 0;
  int32_t n_samples_len = 0;
  int32_t n_samples_30s = 0;
  int32_t n_new_line = 0;

  bool is_microphone = false;
  string token;
  string url = "https://api.deepseek.com/chat/completions";

  float vad_thold = 0.6f;
  float freq_thold = 100.0f;

  bool translate = false;
  bool no_fallback = false;
  bool print_special = false;
  bool no_context = true;
  bool no_timestamps = false;
  bool tinydiarize = false;
  bool save_audio = false; // save audio to wav file
  bool use_gpu = true;
  bool flash_attn = false;
  bool use_vad = false;

  string language = "en";
  string model = "models/ggml-base.en.bin";
  string fname_out;

  bool is_print = false;

  int32_t timeout = 30000;

  string llm = "gemma3:4b";

  string prompt = "";
  string init_prompt = "";
  string system = "";
};

auto whisper_params_parse(int argc, char **argv, whisper_params &params)
    -> bool;
void print_whisper_params(const whisper_params &p);
