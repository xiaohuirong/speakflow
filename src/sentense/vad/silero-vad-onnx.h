#pragma once

#include <cmath> // for rint
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "onnxruntime_cxx_api.h"

using namespace std;

// timestamp_t class: stores the start and end (in samples) of a speech segment.
class timestamp_t {
public:
  int start;
  int end;

  timestamp_t(int start = -1, int end = -1) : start(start), end(end) {}

  timestamp_t &operator=(const timestamp_t &a) {
    start = a.start;
    end = a.end;
    return *this;
  }

  bool operator==(const timestamp_t &a) const {
    return (start == a.start && end == a.end);
  }

  // Returns a formatted string of the timestamp.
  string c_str() const { return format("{start:%08d, end:%08d}", start, end); }

private:
  // Helper function for formatting.
  string format(const char *fmt, ...) const {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    const auto r = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (r < 0)
      return {};
    const size_t len = r;
    if (len < sizeof(buf))
      return string(buf, len);
    string s(len, '\0');
    va_start(args, fmt);
    vsnprintf(s.data(), len + 1, fmt, args);
    va_end(args);
    return s;
  }
};

// VadIterator class: uses ONNX Runtime to detect speech segments.
class VadIterator {
private:
  // ONNX Runtime resources
  Ort::Env env;
  Ort::SessionOptions session_options;
  shared_ptr<Ort::Session> session = nullptr;
  Ort::AllocatorWithDefaultOptions allocator;
  Ort::MemoryInfo memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);

  // ----- Context-related additions -----
  const int context_samples = 64; // For 16kHz, 64 samples are added as context.
  vector<float> _context;         // Holds the last 64 samples from the previous
                                  // chunk (initialized to zero).

  // Original window size (e.g., 32ms corresponds to 512 samples)
  int window_size_samples;
  // Effective window size = window_size_samples + context_samples
  int effective_window_size;

  // Additional declaration: samples per millisecond
  int sr_per_ms;

  // ONNX Runtime input/output buffers
  vector<Ort::Value> ort_inputs;
  vector<const char *> input_node_names = {"input", "state", "sr"};
  vector<float> input;
  unsigned int size_state = 2 * 1 * 128;
  vector<float> _state;
  vector<int64_t> sr;
  int64_t input_node_dims[2] = {};
  const int64_t state_node_dims[3] = {2, 1, 128};
  const int64_t sr_node_dims[1] = {1};
  vector<Ort::Value> ort_outputs;
  vector<const char *> output_node_names = {"output", "stateN"};

  // Model configuration parameters
  int sample_rate;
  float threshold;
  int min_silence_samples;
  int min_silence_samples_at_max_speech;
  int min_speech_samples;
  float max_speech_samples;
  int speech_pad_samples;
  int audio_length_samples;

  // State management
  bool triggered = false;
  unsigned int temp_end = 0;
  unsigned int current_sample = 0;
  int prev_end;
  int next_start = 0;
  vector<timestamp_t> speeches;
  timestamp_t current_speech;

  // Loads the ONNX model.
  void init_onnx_model(const string &model_path);

  // Initializes threading settings.
  void init_engine_threads(int inter_threads, int intra_threads);

  // Resets internal state (_state, _context, etc.)
  void reset_states();

  // Inference: runs inference on one chunk of input data.
  // data_chunk is expected to have window_size_samples samples.
  void predict(const vector<float> &data_chunk);

public:
  // Constructor: sets model path, sample rate, window size (ms), and other
  // parameters. The parameters are set to match the Python version.
  VadIterator(const string ModelPath, int Sample_rate = 16000,
              int windows_frame_size = 32, float Threshold = 0.5,
              int min_silence_duration_ms = 100, int speech_pad_ms = 30,
              int min_speech_duration_ms = 250,
              float max_speech_duration_s = numeric_limits<float>::infinity());

  // Process the entire audio input.
  void process(const vector<float> &input_wav);

  // Returns the detected speech timestamps.
  const vector<timestamp_t> get_speech_timestamps() const { return speeches; }

  // Public method to reset the internal state.
  void reset() { reset_states(); }
};
