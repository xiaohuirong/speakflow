// vad_processor.h
#pragma once

#ifdef USE_SDL_AUDIO
#include "sdlaudio.h"
#elif defined(USE_QT_AUDIO)
#include "qtaudio.h"
#endif
#include "eventbus.h"
#include "silero-vad-onnx.h"
#include <string>
#include <vector>

class Sentense {
public:
  Sentense(const std::string &model_path, std::shared_ptr<EventBus> bus,
           int sample_rate = 16000, int capture_id = -1,
           bool is_microphone = SDL_TRUE);
  ~Sentense();

  auto initialize() -> bool;
  void start();
  void stop();

private:
  void processAudio();
  void checkForSentences();
  [[nodiscard]] auto extractAudioForVAD() const -> vector<float>;

  // Configuration
  const std::string m_model_path;
  const int m_sample_rate;
  const int m_capture_id;
  const SDL_bool m_is_microphone;

// Components
#ifdef USE_SDL_AUDIO
  audio_async m_audio_capture;
#elif defined(USE_QT_AUDIO)
  AudioAsync m_audio_capture;
#endif
  VadIterator m_vad;
  std::shared_ptr<EventBus> eventBus;

  // Buffers and state
  std::vector<float> m_ring_buffer;
  size_t m_buffer_pos = 0;
  size_t m_buffer_fill = 0;
  bool m_running = false;
  std::mutex m_buffer_mutex;

  // Processing parameters
  static constexpr int BUFFER_DURATION_MS = 50000; // 5秒环形缓冲区
  static constexpr int PROCESS_INTERVAL_MS = 2000; // 每2000ms处理一次
  static constexpr int MIN_SENTENCE_GAP_MS = 500;  // 500ms静默视为句子结束
};
