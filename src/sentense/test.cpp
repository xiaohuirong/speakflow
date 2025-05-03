#include "events.h"
#include "sentense.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// 全局原子标志，用于信号处理
std::atomic<bool> g_running{true};

// 信号处理函数
void signalHandler(int signum) {
  std::cout << "\nInterrupt signal (" << signum << ") received.\n";
  g_running = false;
}

struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t chunkSize;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 16;
  uint16_t audioFormat = 1; // PCM
  uint16_t numChannels = 1;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample = 16;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize;
};

void ensureWavDirectoryExists() {
  try {
    if (!fs::exists("wav")) {
      fs::create_directory("wav");
      std::cout << "Created 'wav' directory" << std::endl;
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error creating directory: " << e.what() << std::endl;
  }
}

void saveAsWav(const std::vector<float> &audio, int sampleRate,
               const std::string &filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to create WAV file: " << filename << std::endl;
    return;
  }

  WavHeader header;
  header.sampleRate = sampleRate;
  header.numChannels = 1;
  header.bitsPerSample = 16;
  header.byteRate = sampleRate * header.numChannels * header.bitsPerSample / 8;
  header.blockAlign = header.numChannels * header.bitsPerSample / 8;

  std::vector<int16_t> pcmData(audio.size());
  for (size_t i = 0; i < audio.size(); ++i) {
    float sample = audio[i];
    sample = std::max(-1.0f, std::min(1.0f, sample));
    pcmData[i] = static_cast<int16_t>(sample * 32767.0f);
  }

  header.dataSize = pcmData.size() * sizeof(int16_t);
  header.chunkSize = 36 + header.dataSize;

  file.write(reinterpret_cast<const char *>(&header), sizeof(header));
  file.write(reinterpret_cast<const char *>(pcmData.data()),
             pcmData.size() * sizeof(int16_t));
}

auto getTimestampFilename(const std::string &prefix,
                          const std::string &extension) -> std::string {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;
  ss << "_" << std::setfill('0') << std::setw(3) << ms.count();

  return "wav/" + prefix + ss.str() + "." + extension;
}

auto main() -> int {
  // 注册信号处理函数
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  auto eventBus = std::make_shared<EventBus>();

  ensureWavDirectoryExists();

  Sentense sen("../../models/silero_vad.onnx", eventBus);

  if (!sen.initialize()) {
    std::cerr << "Failed to initialize sen processor" << std::endl;
    return 1;
  }

  eventBus->subscribe<AudioAddedEvent>([](const std::shared_ptr<Event> &event) {
    auto dataEvent = std::static_pointer_cast<AudioAddedEvent>(event);
    auto sentence = dataEvent->audio;
    static int counter = 0;
    if (!g_running)
      return; // 如果收到停止信号，不再处理新句子

    std::cout << "Detected sentence #" << ++counter << " with "
              << sentence.size() << " samples" << std::endl;

    std::string filename = getTimestampFilename("sentence_", "wav");
    saveAsWav(sentence, 16000, filename);

    std::cout << "Saved to: " << filename << std::endl;
  });

  std::cout << "Starting voice detection. Press Ctrl+C to stop..." << std::endl;
  std::cout << "WAV files will be saved in 'wav/' directory" << std::endl;
  eventBus->publish<StartServiceEvent>("sentense");

  // 主循环 - 现在检查g_running标志
  while (g_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << "Stopping voice detection..." << std::endl;
  eventBus->publish<StopServiceEvent>("sentense");
  std::cout << "Program terminated gracefully." << std::endl;
  return 0;
}
