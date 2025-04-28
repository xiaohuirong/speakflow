// vad_processor.cpp
#include "sentense.h"
#include <chrono>
#include <iostream>
#include <thread>

Sentense::Sentense(const std::string &model_path, int sample_rate,
                           int capture_id, bool is_microphone)
    : m_model_path(model_path), m_sample_rate(sample_rate),
      m_capture_id(capture_id),
      m_is_microphone(is_microphone ? SDL_TRUE : SDL_FALSE),
      m_audio_capture(BUFFER_DURATION_MS),
      m_vad(model_path, sample_rate, 32, 0.5, MIN_SENTENCE_GAP_MS, 30, 250) {

  // 计算环形缓冲区大小
  size_t buffer_size = (m_sample_rate * BUFFER_DURATION_MS) / 1000;
  m_ring_buffer.resize(buffer_size);
}

Sentense::~Sentense() { stop(); }

bool Sentense::initialize() {
  if (!m_audio_capture.init(m_capture_id, m_sample_rate, m_is_microphone)) {
    std::cerr << "Failed to initialize audio capture" << std::endl;
    return false;
  }

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
}

void Sentense::setSentenceCallback(SentenceCallback callback) {
  m_callback = callback;
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

void Sentense::checkForSentences() {
  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  if (m_buffer_fill == 0 || !m_callback)
    return;

  // 从缓冲区中提取数据用于VAD处理
  std::vector<float> audio_for_vad;
  audio_for_vad.reserve(m_buffer_fill);

  if (m_buffer_pos >= m_buffer_fill) {
    // 数据没有环绕
    audio_for_vad.assign(m_ring_buffer.begin() + (m_buffer_pos - m_buffer_fill),
                         m_ring_buffer.begin() + m_buffer_pos);
  } else {
    // 数据有环绕
    size_t first_chunk = m_ring_buffer.size() - (m_buffer_fill - m_buffer_pos);
    audio_for_vad.assign(m_ring_buffer.begin() + first_chunk,
                         m_ring_buffer.end());
    audio_for_vad.insert(audio_for_vad.end(), m_ring_buffer.begin(),
                         m_ring_buffer.begin() + m_buffer_pos);
  }

  // 使用VAD处理音频
  m_vad.process(audio_for_vad);
  auto speeches = m_vad.get_speech_timestamps();

  if (speeches.empty())
    return;

  // 处理检测到的语音段
  for (size_t i = 0; i < speeches.size() - 1; i++) {
    const auto &speech = speeches[i];
    if (speech.end - speech.start < (m_sample_rate * 0.1) / 1000) {
      // 忽略太短的语音段（小于100ms）
      continue;
    }

    // 提取完整句子
    std::vector<float> sentence(audio_for_vad.begin() + speech.start,
                                audio_for_vad.begin() + speech.end);

    // 通过回调返回句子
    m_callback(sentence);
  }

  // 保留最后一个可能的未完成句子在缓冲区中
  const auto &last_speech = speeches.back();
  size_t samples_to_keep = audio_for_vad.size() - last_speech.start;

  // 更新缓冲区，只保留最后一个语音段之后的数据
  if (samples_to_keep > 0) {
    std::vector<float> remaining_audio(
        audio_for_vad.begin() + last_speech.start, audio_for_vad.end());

    // 将剩余数据移到缓冲区开头
    std::copy(remaining_audio.begin(), remaining_audio.end(),
              m_ring_buffer.begin());
    m_buffer_pos = remaining_audio.size();
    m_buffer_fill = remaining_audio.size();
  } else {
    m_buffer_pos = 0;
    m_buffer_fill = 0;
  }

  m_vad.reset();
}
