// vad_processor.cpp
#include "sentense.h"
#include "events.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

Sentense::Sentense(const std::string &model_path, std::shared_ptr<EventBus> bus,
                   int sample_rate, int capture_id, bool is_microphone)
    : m_model_path(model_path), eventBus(std::move(bus)),
      m_sample_rate(sample_rate), m_capture_id(capture_id),
      m_is_microphone(is_microphone ? SDL_TRUE : SDL_FALSE),
      m_audio_capture(BUFFER_DURATION_MS),
      m_vad(model_path, sample_rate, 32, 0.5, MIN_SENTENCE_GAP_MS, 30, 250) {

  // 计算环形缓冲区大小
  size_t buffer_size = (m_sample_rate * BUFFER_DURATION_MS) / 1000;
  m_ring_buffer.resize(buffer_size);

  eventBus->subscribe<StartServiceEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto startEvent = std::static_pointer_cast<StartServiceEvent>(event);
        if (startEvent->serviceName == "sentense") {
          start();
          eventBus->publish<ServiceStatusEvent>("sentense", true);
        }
      });

  eventBus->subscribe<StopServiceEvent>(
      [this](const std::shared_ptr<Event> &event) {
        auto stopEvent = std::static_pointer_cast<StopServiceEvent>(event);
        if (stopEvent->serviceName == "sentense") {
          stop();
          eventBus->publish<ServiceStatusEvent>("sentence", false);
        }
      });
}

Sentense::~Sentense() { stop(); }

auto Sentense::initialize() -> bool {

#ifdef USE_SDL_AUDIO
  if (!m_audio_capture.init(m_capture_id, m_sample_rate, m_is_microphone)) {
    std::cerr << "Failed to initialize audio capture" << std::endl;
    return false;
  }
#elif defined(USE_QT_AUDIO)
  if (!m_audio_capture.init(-1, WHISPER_SAMPLE_RATE, true)) {
    std::cerr << "Failed to initialize audio capture" << std::endl;
    return false;
  }
#endif

  return true;
}

void Sentense::start() {
  if (m_running)
    return;

  m_running = true;
  m_audio_capture.resume();

  // 启动处理线程
  std::thread([this]() {
    while (m_running) {
      auto start = std::chrono::steady_clock::now();

      // 处理音频数据
      processAudio();

      // 检查是否有完整句子
      checkForSentences();

      // 等待直到下一个处理周期
      auto end = std::chrono::steady_clock::now();
      auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      if (elapsed.count() < PROCESS_INTERVAL_MS) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(PROCESS_INTERVAL_MS - elapsed.count()));
      }
    }
  }).detach();
}

void Sentense::stop() {
  m_running = false;
  m_audio_capture.pause();

  // 处理残留音频
  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  if (m_buffer_fill == 0 || !eventBus) {
    m_buffer_pos = 0;
    return;
  }

  // 从缓冲区中提取数据用于VAD处理
  std::vector<float> audio_for_vad = extractAudioForVAD();

  // 使用VAD处理音频
  m_vad.process(audio_for_vad);
  auto speeches = m_vad.get_speech_timestamps();

  if (!speeches.empty()) {
    std::vector<float> sentence(audio_for_vad.begin() + speeches[0].start,
                                audio_for_vad.begin() + speeches[0].end);
    eventBus->publish<AudioAddedEvent>(sentence);
  }

  m_buffer_fill = 0;
  m_buffer_pos = 0;
  m_vad.reset();
}

void Sentense::processAudio() {
  // 从音频捕获获取最新数据
  std::vector<float> new_audio;
  m_audio_capture.get(PROCESS_INTERVAL_MS, new_audio);

  if (new_audio.empty())
    return;

  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  // 将新数据添加到环形缓冲区
  size_t samples_to_add = new_audio.size();
  if (samples_to_add > m_ring_buffer.size()) {
    // 如果新数据比整个缓冲区还大，只保留最后的部分
    new_audio.erase(new_audio.begin(), new_audio.end() - m_ring_buffer.size());
    samples_to_add = m_ring_buffer.size();
  }

  // 计算写入位置
  size_t write_pos = m_buffer_pos;
  size_t first_chunk =
      std::min(samples_to_add, m_ring_buffer.size() - write_pos);
  size_t second_chunk = samples_to_add - first_chunk;

  // 写入数据
  std::copy(new_audio.begin(), new_audio.begin() + first_chunk,
            m_ring_buffer.begin() + write_pos);

  if (second_chunk > 0) {
    std::copy(new_audio.begin() + first_chunk, new_audio.end(),
              m_ring_buffer.begin());
  }

  // 更新位置和填充量
  m_buffer_pos = (write_pos + samples_to_add) % m_ring_buffer.size();
  m_buffer_fill =
      std::min(m_buffer_fill + samples_to_add, m_ring_buffer.size());
}

// unsafe operation
auto Sentense::extractAudioForVAD() const -> vector<float> {
  vector<float> audio_for_vad;
  audio_for_vad.reserve(m_buffer_fill);

  if (m_buffer_pos >= m_buffer_fill) {
    audio_for_vad.assign(m_ring_buffer.begin() + (m_buffer_pos - m_buffer_fill),
                         m_ring_buffer.begin() + m_buffer_pos);
  } else {
    size_t first_chunk = m_ring_buffer.size() - (m_buffer_fill - m_buffer_pos);
    audio_for_vad.assign(m_ring_buffer.begin() + first_chunk,
                         m_ring_buffer.end());
    audio_for_vad.insert(audio_for_vad.end(), m_ring_buffer.begin(),
                         m_ring_buffer.begin() + m_buffer_pos);
  }

  return std::move(audio_for_vad); // 显式移动
}

void Sentense::checkForSentences() {
  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  if (m_buffer_fill == 0 || !eventBus)
    return;

  // 从缓冲区中提取数据用于VAD处理
  std::vector<float> audio_for_vad = extractAudioForVAD();

  // 使用VAD处理音频
  m_vad.process(audio_for_vad);
  auto speeches = m_vad.get_speech_timestamps();

  if (speeches.empty()) {
    m_buffer_fill = 0;
    return;
  }

  size_t start = speeches[0].start;
  size_t end = speeches[0].end;

  // 处理检测到的语音段
  for (size_t i = 0; i < speeches.size() - 1; i++) {
    const auto &speech = speeches[i];
    if (speech.end - speech.start < (m_sample_rate * 100) / 1000) {
      // 忽略太短的语音段（小于100ms）
      continue;
    }
    end = speech.end;

    // 提取完整句子
    std::vector<float> sentence(audio_for_vad.begin() + start,
                                audio_for_vad.begin() + end);

    // 通过回调返回句子
    eventBus->publish<AudioAddedEvent>(sentence);

    start = speeches[i + 1].start;
    end = speeches[i + 1].end;
  }

  if (m_buffer_fill - end >= PROCESS_INTERVAL_MS * m_sample_rate / 1000) {
    std::vector<float> sentence(audio_for_vad.begin() + start,
                                audio_for_vad.begin() + end);
    eventBus->publish<AudioAddedEvent>(sentence);

    m_buffer_fill = 0;
    m_buffer_pos = 0;
  } else {
    m_buffer_fill -= start;
  }

  m_vad.reset();
}
