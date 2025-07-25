#define _USE_MATH_DEFINES // for M_PI

#include "common-whisper.h"

#include <whisper.h>

// third-party utilities
// use your favorite implementations
#include "stb/stb_vorbis.c" /* Enables Vorbis decoding. */

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define MA_NO_DEVICE_IO
#define MA_NO_THREADING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MINIAUDIO_IMPLEMENTATION
#include "faust/miniaudio.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4244 4267) // possible loss of data
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <cstring>
#include <fstream>
#include <print>

#ifdef WHISPER_FFMPEG
// as implemented in ffmpeg_trancode.cpp only embedded in common lib if whisper
// built with ffmpeg support
extern bool ffmpeg_decode_audio(const string &ifname,
                                vector<uint8_t> &wav_data);
#endif

auto read_audio_data(const string &fname, vector<float> &pcmf32,
                     vector<vector<float>> &pcmf32s, bool stereo) -> bool {
  vector<uint8_t>
      audio_data; // used for pipe input from stdin or ffmpeg decoding output

  ma_result result;
  ma_decoder_config decoder_config;
  ma_decoder decoder;

  decoder_config = ma_decoder_config_init(ma_format_f32, stereo ? 2 : 1,
                                          WHISPER_SAMPLE_RATE);

  if (fname == "-") {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    uint8_t buf[1024];
    while (true) {
      const size_t n = fread(buf, 1, sizeof(buf), stdin);
      if (n == 0) {
        break;
      }
      audio_data.insert(audio_data.end(), buf, buf + n);
    }

    if ((result = ma_decoder_init_memory(audio_data.data(), audio_data.size(),
                                         &decoder_config, &decoder)) !=
        MA_SUCCESS) {

      println(stderr, "Error: failed to open audio data from stdin ({})",
              ma_result_description(result));

      return false;
    }

    println(stderr, "{}: read {} bytes from stdin", __func__,
            audio_data.size());
  } else if (((result = ma_decoder_init_file(fname.c_str(), &decoder_config,
                                             &decoder)) != MA_SUCCESS)) {
#if defined(WHISPER_FFMPEG)
    if (ffmpeg_decode_audio(fname, audio_data) != 0) {
      fprintf(stderr, "error: failed to ffmpeg decode '%s'\n", fname.c_str());

      return false;
    }

    if ((result = ma_decoder_init_memory(audio_data.data(), audio_data.size(),
                                         &decoder_config, &decoder)) !=
        MA_SUCCESS) {
      fprintf(stderr, "error: failed to read audio data as wav (%s)\n",
              ma_result_description(result));

      return false;
    }
#else
    if ((result = ma_decoder_init_memory(fname.c_str(), fname.size(),
                                         &decoder_config, &decoder)) !=
        MA_SUCCESS) {
      println(stderr, "error: failed to read audio data as wav ({})",
              ma_result_description(result));

      return false;
    }
#endif
  }

  ma_uint64 frame_count;
  ma_uint64 frames_read;

  if ((result = ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count)) !=
      MA_SUCCESS) {
    println(stderr,
            "error: failed to retrieve the length of the audio data ({})",
            ma_result_description(result));

    return false;
  }

  pcmf32.resize(stereo ? frame_count * 2 : frame_count);

  if ((result = ma_decoder_read_pcm_frames(&decoder, pcmf32.data(), frame_count,
                                           &frames_read)) != MA_SUCCESS) {
    println(stderr, "error: failed to read the frames of the audio data ({})",
            ma_result_description(result));

    return false;
  }

  if (stereo) {
    pcmf32s.resize(2);
    pcmf32s[0].resize(frame_count);
    pcmf32s[1].resize(frame_count);
    for (uint64_t i = 0; i < frame_count; i++) {
      pcmf32s[0][i] = pcmf32[2 * i];
      pcmf32s[1][i] = pcmf32[2 * i + 1];
    }
  }

  ma_decoder_uninit(&decoder);

  return true;
}

//  500 -> 00:05.000
// 6000 -> 01:00.000
auto to_timestamp(int64_t t, bool comma) -> string {
  int64_t msec = t * 10;
  int64_t hr = msec / (1000 * 60 * 60);
  msec = msec - hr * (1000 * 60 * 60);
  int64_t min = msec / (1000 * 60);
  msec = msec - min * (1000 * 60);
  int64_t sec = msec / 1000;
  msec = msec - sec * 1000;

  char buf[32];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int)hr, (int)min,
           (int)sec, comma ? "," : ".", (int)msec);

  return string(buf);
}

auto timestamp_to_sample(int64_t t, int n_samples, int whisper_sample_rate)
    -> int {
  return max(0,
             min((int)n_samples - 1, (int)((t * whisper_sample_rate) / 100)));
}

auto speak_with_file(const string &command, const string &text,
                     const string &path, int voice_id) -> bool {
  ofstream speak_file(path.c_str());
  if (speak_file.fail()) {
    println(stderr, "{}: failed to open speak_file", __func__);
    return false;
  } else {
    speak_file.write(text.c_str(), text.size());
    speak_file.close();
    int ret =
        system((command + " " + to_string(voice_id) + " " + path).c_str());
    if (ret != 0) {
      println(stderr, "{}: failed to speak", __func__);
      return false;
    }
  }
  return true;
}
