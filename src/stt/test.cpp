#include "common_stt.h"
#include "events.h"
#include "stt.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>
#include <whisper.h>

using std::vector;

namespace fs = std::filesystem;

using WhisperCallback = std::function<void(const std::string &)>;

struct WavHeader {
  char riff[4];
  uint32_t chunkSize;
  char wave[4];
  char fmt[4];
  uint32_t fmtSize;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  char data[4];
  uint32_t dataSize;
};

auto readWavFilesFromDirectory(const std::string &directoryPath)
    -> std::vector<std::vector<float>> {
  std::vector<std::vector<float>> allAudioData;

  try {
    for (const auto &entry : fs::directory_iterator(directoryPath)) {
      if (entry.path().extension() == ".wav") {
        std::ifstream file(entry.path(), std::ios::binary);
        if (!file) {
          spdlog::error("Failed to open WAV file: {}", entry.path().string());
          continue;
        }

        WavHeader header;
        file.read(reinterpret_cast<char *>(&header), sizeof(header));

        // Basic WAV format validation
        if (std::string(header.riff, 4) != "RIFF" ||
            std::string(header.wave, 4) != "WAVE" ||
            std::string(header.fmt, 4) != "fmt " ||
            std::string(header.data, 4) != "data") {
          spdlog::error("Invalid WAV file format: {}", entry.path().string());
          continue;
        }

        // Only support 16-bit PCM for this example
        if (header.audioFormat != 1 || header.bitsPerSample != 16) {
          spdlog::error(
              "Unsupported WAV format (only 16-bit PCM supported): {}",
              entry.path().string());
          continue;
        }

        size_t sampleCount = header.dataSize / (header.bitsPerSample / 8);
        std::vector<int16_t> pcmData(sampleCount);
        file.read(reinterpret_cast<char *>(pcmData.data()), header.dataSize);

        // Convert to float32 in range [-1.0, 1.0]
        std::vector<float> floatData(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
          floatData[i] = static_cast<float>(pcmData[i]) / 32768.0f;
        }

        allAudioData.push_back(floatData);
        spdlog::info("Read WAV file: {} with {} samples", entry.path().string(),
                     sampleCount);
      }
    }
  } catch (const fs::filesystem_error &e) {
    spdlog::error("Filesystem error: {}", e.what());
  }

  return allAudioData;
}

auto main() -> int {
  std::string wavDirectory = "../sentense/wav";

  if (!fs::exists(wavDirectory)) {
    spdlog::error("Directory '{}' does not exist.", wavDirectory);
    return 1;
  }

  std::vector<std::vector<float>> audioData =
      readWavFilesFromDirectory(wavDirectory);

  spdlog::info("Successfully read {} WAV files.", audioData.size());

  // Example: Print info about the first file if available
  if (!audioData.empty()) {
    spdlog::info("First file has {} samples.", audioData[0].size());
  }

  whisper_context_params cparams(whisper_context_default_params());
  whisper_full_params wparams(
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY));

  cparams.use_gpu = true;
  cparams.flash_attn = false;

  wparams.print_progress = false;
  wparams.print_special = false;
  wparams.print_realtime = false;
  wparams.print_timestamps = true;
  wparams.translate = false;
  wparams.single_segment = false;
  wparams.max_tokens = 0;
  wparams.n_threads = 6;
  wparams.beam_search.beam_size = -1;
  wparams.audio_ctx = 0;
  wparams.tdrz_enable = false;

  string model = "../../models/ggml-large-v3.bin";
  string language = "zh";

  auto eventBus = std::make_shared<EventBus>();

  WhisperCallback whisperCallback(
      [](const string &text) { spdlog::info(text); });

  STT stt(cparams, wparams, model, language, false, whisperCallback, eventBus);
  STTOperations sttcallbacks = stt.getOperations();

  // 初始化测试用的前端回调函数
  const CardWidgetCallbacks frontendCallbacks{
      // 当音频被添加时的回调
      .onVoiceAdded = [](const string &length) { spdlog::info(length); },

      // 当音频被移除时的回调
      .onVoiceRemoved = [](size_t index) { spdlog::info("remove {}", index); },

      // 当音频列表被清空时的回调
      .onVoiceCleared = []() { spdlog::info("clear"); },

      // 当触发方法改变时的回调
      .onTriggerMethodChanged =
          [](TriggerMethod method) {
            std::string methodStr;
            switch (method) {
            case AUTO_TRIGGER:
              methodStr = "AUTO_TRIGGER";
              break;
            case NO_TRIGGER:
              methodStr = "NO_TRIGGER";
              break;
            case ONCE_TRIGGER:
              methodStr = "ONCE_TRIGGER";
              break;
            default:
              methodStr = "UNKNOWN";
              break;
            }
            spdlog::info(methodStr);
          }};

  stt.setCardWidgetCallbacks(frontendCallbacks);

  stt.start();
  spdlog::info("stt start");

  // wait thread into lock
  // std::this_thread::sleep_for(std::chrono::milliseconds(100));

  spdlog::info("###########manual trigger test############");
  sttcallbacks.setTriggerMethod(NO_TRIGGER);

  spdlog::info("stt set auto process 0");

  sttcallbacks.addVoice(audioData[0]);
  spdlog::info("stt addVoice 0");

  sttcallbacks.addVoice(audioData[1]);
  spdlog::info("stt addVoice 1");

  sttcallbacks.removeVoice(1);
  spdlog::info("stt remove index 1");

  sttcallbacks.setTriggerMethod(ONCE_TRIGGER);
  spdlog::info("stt process once");

  // std::this_thread::sleep_for(std::chrono::milliseconds(100));

  spdlog::info("###########auto trigger test############");
  sttcallbacks.addVoice(audioData[2]);
  spdlog::info("stt addVoice 2");
  // stt.setTriggerMethod(-1);
  // spdlog::info("stt set auto process -1");

  sttcallbacks.addVoice(audioData[2]);
  spdlog::info("stt addVoice 2");

  sttcallbacks.addVoice(audioData[3]);
  spdlog::info("stt addVoice 3");

  // stt.setTriggerMethod(1);
  // spdlog::info("stt process once");

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  stt.stop();
  spdlog::info("stt stop");

  return 0;
}
